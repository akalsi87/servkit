/**
 * \file net.c
 * \date Jan 18, 2017
 */

#include <servkit/net.h>

#include <servkit/asserts.h>

/* Flags used with certain functions. */
#define SK_NET_NONE 0
#define SK_NET_IP_ONLY (1<<0)

#if defined(__sun) || defined(_AIX)
#define AF_LOCAL AF_UNIX
#endif

#ifdef _AIX
#undef ip_len
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

static
void skNetSetError(char* err, const char* fmt, ...)
{
    va_list ap;

    if (!err) {
        return;
    }
    va_start(ap, fmt);
    vsnprintf(err, SK_NET_ERR_LEN, fmt, ap);
    va_end(ap);
    skDbgTraceF(SK_LVL_ERR, "%s", err);
}

int skNetSetBlock(char* err, int fd, int non_block)
{
    int flags;

    /* Set the socket blocking (if non_block is zero) or non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        skNetSetError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return SK_NET_ERR;
    }

    if (non_block) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (fcntl(fd, F_SETFL, flags) == -1) {
        skNetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

int skNetNonBlock(char* err, int fd)
{
    return skNetSetBlock(err,fd,1);
}

int skNetBlock(char* err, int fd)
{
    return skNetSetBlock(err,fd,0);
}

/* Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
int skNetKeepAlive(char* err, int fd, int interval)
{
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
        skNetSetError(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return SK_NET_ERR;
    }

#ifdef __linux__
    /* Default settings are more or less garbage, with the keepalive time
     * set to 7200 by default on Linux. Modify settings to make the feature
     * actually useful. */

    /* Send first probe after interval. */
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        skNetSetError(err, "setsockopt TCP_KEEPIDLE: %s\n", strerror(errno));
        return SK_NET_ERR;
    }

    /* Send next probes after the specified interval. Note that we set the
     * delay as interval / 3, as we send three probes before detecting
     * an error (see the next setsockopt call). */
    val = interval/3;
    if (val == 0) {
        val = 1;
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        skNetSetError(err, "setsockopt TCP_KEEPINTVL: %s\n", strerror(errno));
        return SK_NET_ERR;
    }

    /* Consider the socket in error state after three
     * we send three ACK probes without getting a reply. */
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        skNetSetError(err, "setsockopt TCP_KEEPCNT: %s\n", strerror(errno));
        return SK_NET_ERR;
    }
#else
    ((void) interval); /* Avoid unused var warning for non Linux systems. */
#endif

    return SK_NET_OK;
}

static
int skNetSetTcpNoDelay(char* err, int fd, int val)
{
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
        skNetSetError(err, "setsockopt TCP_NODELAY: %s", strerror(errno));
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

int skNetEnableTcpNoDelay(char* err, int fd)
{
    return skNetSetTcpNoDelay(err, fd, 1);
}

int skNetDisableTcpNoDelay(char* err, int fd)
{
    return skNetSetTcpNoDelay(err, fd, 0);
}


int skNetSetSendBuffer(char* err, int fd, int buffsize)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1) {
        skNetSetError(err, "setsockopt SO_SNDBUF: %s", strerror(errno));
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

int skNetTcpKeepAlive(char* err, int fd)
{
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        skNetSetError(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

/* Set the socket send timeout (SO_SNDTIMEO socket option) to the specified
 * number of milliseconds, or disable it if the 'ms' argument is zero. */
int skNetSendTimeout(char* err, int fd, sk_ll ms)
{
    struct timeval tv;

    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        skNetSetError(err, "setsockopt SO_SNDTIMEO: %s", strerror(errno));
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

/* skNetGenericResolve() is called by skNetResolve() and skNetResolveIP() to
 * do the actual work. It resolves the hostname "host" and set the string
 * representation of the IP address into the buffer pointed by "ipbuf".
 *
 * If flags is set to SK_NET_IP_ONLY the function only resolves hostnames
 * that are actually already IPv4 or IPv6 addresses. This turns the function
 * into a validating / normalizing function. */
int skNetGenericResolve(char* err, char* host, char* ipbuf, size_t ipbuf_len,
                       int flags)
{
    struct addrinfo hints, *info;
    int rv;

    memset(&hints,0,sizeof(hints));
    if (flags & SK_NET_IP_ONLY) hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  /* specify socktype to avoid dups */

    if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
        skNetSetError(err, "%s", gai_strerror(rv));
        return SK_NET_ERR;
    }
    if (info->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
    } else {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)info->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
    }

    freeaddrinfo(info);
    return SK_NET_OK;
}

int skNetResolve(char* err, char* host, char* ipbuf, size_t ipbuf_len) {
    return skNetGenericResolve(err,host,ipbuf,ipbuf_len,SK_NET_NONE);
}

int skNetResolveIP(char* err, char* host, char* ipbuf, size_t ipbuf_len) {
    return skNetGenericResolve(err,host,ipbuf,ipbuf_len,SK_NET_IP_ONLY);
}

static
int skNetSetReuseAddr(char* err, int fd)
{
    int yes = 1;
    /* Make sure connection-intensive things like the redis benckmark
     * will be able to close/open sockets a zillion of times */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        skNetSetError(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

static
int skNetCreateSocket(char* err, int domain)
{
    int s;
    if ((s = socket(domain, SOCK_STREAM, 0)) == -1) {
        skNetSetError(err, "creating socket: %s", strerror(errno));
        return SK_NET_ERR;
    }

    /* Make sure connection-intensive things like the redis benchmark
     * will be able to close/open sockets a zillion of times */
    if (skNetSetReuseAddr(err,s) == SK_NET_ERR) {
        close(s);
        return SK_NET_ERR;
    }
    return s;
}

#define SK_NET_CONNECT_NONE 0
#define SK_NET_CONNECT_NONBLOCK 1
#define SK_NET_CONNECT_BE_BINDING 2 /* Best effort binding. */

static
int skNetTcpGenericConnect(char* err, char* addr, int port,
                           char* source_addr, int flags)
{
    int s = SK_NET_ERR, rv;
    char portstr[6];  /* strlen("65535") + 1; */
    struct addrinfo hints, *servinfo, *bservinfo, *p, *b;

    snprintf(portstr,sizeof(portstr),"%d",port);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr,portstr,&hints,&servinfo)) != 0) {
        skNetSetError(err, "%s", gai_strerror(rv));
        return SK_NET_ERR;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* Try to create the socket and to connect it.
         * If we fail in the socket() call, or on connect(), we retry with
         * the next entry in servinfo. */
        if ((s = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1) {
            continue;
        }
        if (skNetSetReuseAddr(err,s) == SK_NET_ERR) {
            goto error;
        }
        if ((flags & SK_NET_CONNECT_NONBLOCK) && (skNetNonBlock(err,s) != SK_NET_OK)) {
            goto error;
        }
        if (source_addr) {
            int bound = 0;
            /* Using getaddrinfo saves us from self-determining IPv4 vs IPv6 */
            if ((rv = getaddrinfo(source_addr, NULL, &hints, &bservinfo)) != 0) {
                skNetSetError(err, "%s", gai_strerror(rv));
                goto error;
            }
            for (b = bservinfo; b != NULL; b = b->ai_next) {
                if (bind(s,b->ai_addr,b->ai_addrlen) != -1) {
                    bound = 1;
                    break;
                }
            }
            freeaddrinfo(bservinfo);
            if (!bound) {
                skNetSetError(err, "bind: %s", strerror(errno));
                goto error;
            }
        }
        if (connect(s,p->ai_addr,p->ai_addrlen) == -1) {
            /* If the socket is non-blocking, it is ok for connect() to
             * return an EINPROGRESS error here. */
            if (errno == EINPROGRESS && flags & SK_NET_CONNECT_NONBLOCK)
                goto end;
            close(s);
            s = SK_NET_ERR;
            continue;
        }

        /* If we ended an iteration of the for loop without errors, we
         * have a connected socket. Let's return to the caller. */
        goto end;
    }
    if (p == NULL)
        skNetSetError(err, "creating socket: %s", strerror(errno));

error:
    if (s != SK_NET_ERR) {
        close(s);
        s = SK_NET_ERR;
    }

end:
    freeaddrinfo(servinfo);

    /* Handle best effort binding: if a binding address was used, but it is
     * not possible to create a socket, try again without a binding address. */
    if (s == SK_NET_ERR && source_addr && (flags & SK_NET_CONNECT_BE_BINDING)) {
        return skNetTcpGenericConnect(err,addr,port,NULL,flags);
    } else {
        return s;
    }
}

int skNetTcpConnect(char* err, char* addr, int port)
{
    return skNetTcpGenericConnect(err,addr,port,NULL,SK_NET_CONNECT_NONE);
}

int skNetTcpNonBlockConnect(char* err, char* addr, int port)
{
    return skNetTcpGenericConnect(err,addr,port,NULL,SK_NET_CONNECT_NONBLOCK);
}

int skNetTcpNonBlockBindConnect(char* err, char* addr, int port,
                               char* source_addr)
{
    return skNetTcpGenericConnect(err,addr,port,source_addr,
            SK_NET_CONNECT_NONBLOCK);
}

int skNetTcpNonBlockBestEffortBindConnect(char* err, char* addr, int port,
                                         char* source_addr)
{
    return skNetTcpGenericConnect(err,addr,port,source_addr,
            SK_NET_CONNECT_NONBLOCK|SK_NET_CONNECT_BE_BINDING);
}

int skNetUnixGenericConnect(char* err, char* path, int flags)
{
    int s;
    struct sockaddr_un sa;

    if ((s = skNetCreateSocket(err,AF_LOCAL)) == SK_NET_ERR) {
        return SK_NET_ERR;
    }

    sa.sun_family = AF_LOCAL;
    strncpy(sa.sun_path,path,sizeof(sa.sun_path)-1);
    if (flags & SK_NET_CONNECT_NONBLOCK) {
        if (skNetNonBlock(err,s) != SK_NET_OK) {
            return SK_NET_ERR;
        }
    }
    if (connect(s,(struct sockaddr*)&sa,sizeof(sa)) == -1) {
        if (errno == EINPROGRESS && (flags & SK_NET_CONNECT_NONBLOCK)) {
            return s;
        }
        skNetSetError(err, "connect: %s", strerror(errno));
        close(s);
        return SK_NET_ERR;
    }
    return s;
}

int skNetUnixConnect(char* err, char* path)
{
    return skNetUnixGenericConnect(err,path,SK_NET_CONNECT_NONE);
}

int skNetUnixNonBlockConnect(char* err, char* path)
{
    return skNetUnixGenericConnect(err,path,SK_NET_CONNECT_NONBLOCK);
}

/* Like read(2) but make sure 'count' is read before to return
 * (unless error or EOF condition is encountered) */
int skNetRead(int fd, char* buf, int count)
{
    ssize_t nRead, totalLen = 0;
    while(totalLen != count) {
        nRead = read(fd,buf,count-totalLen);
        if (nRead == 0) {
            return totalLen;
        }
        if (nRead == -1) {
            return -1;
        }
        totalLen += nRead;
        buf += nRead;
    }
    return totalLen;
}

/* Like write(2) but make sure 'count' is written before to return
 * (unless error is encountered) */
int skNetWrite(int fd, char* buf, int count)
{
    ssize_t nWritten, totalLen = 0;
    while(totalLen != count) {
        nWritten = write(fd,buf,count-totalLen);
        if (nWritten == 0) {
            return totalLen;
        }
        if (nWritten == -1) {
            return -1;
        }
        totalLen += nWritten;
        buf += nWritten;
    }
    return totalLen;
}

static
int skNetListen(char* err, int s, struct sockaddr *sa, socklen_t len, int backlog)
{
    if (bind(s,sa,len) == -1) {
        skNetSetError(err, "bind: %s", strerror(errno));
        close(s);
        return SK_NET_ERR;
    }

    if (listen(s, backlog) == -1) {
        skNetSetError(err, "listen: %s", strerror(errno));
        close(s);
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

static
int skNetV6Only(char* err, int s) {
    int yes = 1;
    if (setsockopt(s,IPPROTO_IPV6,IPV6_V6ONLY,&yes,sizeof(yes)) == -1) {
        skNetSetError(err, "setsockopt: %s", strerror(errno));
        close(s);
        return SK_NET_ERR;
    }
    return SK_NET_OK;
}

static
int _skNetTcpServer(char* err, int port, char* bindAddr, int af, int backlog)
{
    int s, rv;
    char _port[6];  /* strlen("65535") */
    struct addrinfo hints, *servinfo, *p;

    snprintf(_port,6,"%d",port);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    /* No effect if bindAddr != NULL */

    if ((rv = getaddrinfo(bindAddr,_port,&hints,&servinfo)) != 0) {
        skNetSetError(err, "%s", gai_strerror(rv));
        return SK_NET_ERR;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((s = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1) {
            continue;
        }

        if (af == AF_INET6 && skNetV6Only(err,s) == SK_NET_ERR) {
            goto error;
        }
        if (skNetSetReuseAddr(err,s) == SK_NET_ERR) {
            goto error;
        }
        if (skNetListen(err,s,p->ai_addr,p->ai_addrlen,backlog) == SK_NET_ERR) {
            goto error;
        }
        goto end;
    }
    if (p == NULL) {
        skNetSetError(err, "unable to bind socket, errno: %d", errno);
        goto error;
    }

error:
    s = SK_NET_ERR;
end:
#ifndef NDEBUG
    if (s != SK_NET_ERR) {
        skTraceF(SK_LVL_SUCC, "created TCP server fd=%d at port %d, bindAddr %s, backlog %d", s, port, bindAddr ? bindAddr : "", backlog);
    }
#endif
    freeaddrinfo(servinfo);
    return s;
}

int skNetTcpServer(char* err, int port, char* bindaddr, int backlog)
{
    return _skNetTcpServer(err, port, bindaddr, AF_INET, backlog);
}

int skNetTcp6Server(char* err, int port, char* bindaddr, int backlog)
{
    return _skNetTcpServer(err, port, bindaddr, AF_INET6, backlog);
}

int skNetUnixServer(char* err, char* path, mode_t perm, int backlog)
{
    int s;
    struct sockaddr_un sa;

    if ((s = skNetCreateSocket(err,AF_LOCAL)) == SK_NET_ERR) {
        return SK_NET_ERR;
    }

    memset(&sa,0,sizeof(sa));
    sa.sun_family = AF_LOCAL;
    strncpy(sa.sun_path,path,sizeof(sa.sun_path)-1);
    if (skNetListen(err,s,(struct sockaddr*)&sa,sizeof(sa),backlog) == SK_NET_ERR) {
        return SK_NET_ERR;
    }
    if (perm) {
        chmod(sa.sun_path, perm);
    }
    return s;
}

static
int skNetGenericAccept(char* err, int s, struct sockaddr *sa, socklen_t *len)
{
    int fd;
    while(1) {
        fd = accept(s,sa,len);
        if (fd == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                skNetSetError(err, "accept: %s", strerror(errno));
                return SK_NET_ERR;
            }
        }
        break;
    }
    return fd;
}

int skNetTcpAccept(char* err, int s, char* ip, size_t ip_len, int* port)
{
    int fd;
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);
    if ((fd = skNetGenericAccept(err,s,(struct sockaddr*)&sa,&salen)) == -1)
        return SK_NET_ERR;

    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) {
            inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        }
        if (port) {
            *port = ntohs(s->sin_port);
        }
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) {
            inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        }
        if (port) {
            *port = ntohs(s->sin6_port);
        }
    }
    return fd;
}

int skNetUnixAccept(char* err, int s)
{
    int fd;
    struct sockaddr_un sa;
    socklen_t salen = sizeof(sa);
    if ((fd = skNetGenericAccept(err,s,(struct sockaddr*)&sa,&salen)) == -1) {
        return SK_NET_ERR;
    }

    return fd;
}

int skNetPeerToString(int fd, char* ip, size_t ip_len, int* port)
{
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if (getpeername(fd,(struct sockaddr*)&sa,&salen) == -1) {
        goto error;
    }
    if (ip_len == 0) {
        goto error;
    }

    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) {
            inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        }
        if (port) {
            *port = ntohs(s->sin_port);
        }
    } else if (sa.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) {
            inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        }
        if (port) {
            *port = ntohs(s->sin6_port);
        }
    } else if (sa.ss_family == AF_UNIX) {
        if (ip) {
            strncpy(ip,"/unixsocket",ip_len);
        }
        if (port) {
            *port = 0;
        }
    } else {
        goto error;
    }
    return 0;

error:
    if (ip) {
        if (ip_len >= 2) {
            ip[0] = '?';
            ip[1] = '\0';
        } else if (ip_len == 1) {
            ip[0] = '\0';
        }
    }
    if (port) {
        *port = 0;
    }
    return -1;
}

/* Format an IP,port pair into something easy to parse. If IP is IPv6
 * (matches for ":"), the ip is surrounded by []. IP and port are just
 * separated by colons. This the standard to display addresses within Redis. */
int skNetFormatAddr(char* buf, size_t buf_len, char* ip, int port)
{
    return snprintf(buf,buf_len, strchr(ip,':') ?
           "[%s]:%d" : "%s:%d", ip, port);
}

/* Like skNetFormatAddr() but extract ip and port from the socket's peer. */
int skNetFormatPeer(int fd, char* buf, size_t buf_len)
{
    char ip[INET6_ADDRSTRLEN];
    int port;

    skNetPeerToString(fd,ip,sizeof(ip),&port);
    return skNetFormatAddr(buf, buf_len, ip, port);
}

int skNetSockName(int fd, char* ip, size_t ip_len, int* port)
{
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if (getsockname(fd,(struct sockaddr*)&sa,&salen) == -1) {
        if (port) *port = 0;
        ip[0] = '?';
        ip[1] = '\0';
        return -1;
    }
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) {
            inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        }
        if (port) {
            *port = ntohs(s->sin_port);
        }
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) {
            inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        }
        if (port) {
            *port = ntohs(s->sin6_port);
        }
    }
    return 0;
}

int skNetFormatSock(int fd, char* fmt, size_t fmt_len)
{
    char ip[INET6_ADDRSTRLEN];
    int port;

    skNetSockName(fd,ip,sizeof(ip),&port);
    return skNetFormatAddr(fmt, fmt_len, ip, port);
}

int skNetClose(char* err, int fd)
{
    int rv = close(fd);
    if (rv != 0) {
        skAssertMsg(rv != EBADF, "Invalid file descriptor provided.");
        if (rv == EINTR) {
            skNetSetError(err, "close(fd) with fd=%d failed when interrupted by signal().", fd);
        } else {
            skAssert(rv == EIO);
            skNetSetError(err, "close(fd) with fd=%d failed with EIO.", fd);
        }
        rv = SK_NET_ERR;
    }
#ifndef NDEBUG
    else {
        skTraceF(SK_LVL_SUCC, "close(fd) with fd=%d succeeded.", fd);
    }
#endif
    return rv;
}
