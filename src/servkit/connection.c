/**
 * \file connection.c
 * \date Jan 19, 2017
 */

#include <servkit/connection.h>
#include <servkit/asserts.h>

#include <string.h>

static
void skFillOOMError(char* err)
{
    skDbgTraceF(SK_LVL_ERR, "Out of memory.");
    strcpy(err, "connInit: out of memory.");
}

int skConnInitTcpClient(char* err, skConn* conn, char const* serverName, int port)
{
    int s = skNetTcpConnect(err, serverName, port);
    if (s != SK_NET_ERR) {
        char const* srvName = strdup(serverName);
        if (!srvName) {
            skFillOOMError(err);
            return SK_CONN_ERR;
        }
        skDbgTraceF(SK_LVL_SUCC, "TCP client fd=%d established to %s:%d.", s, serverName, port);
        conn->kind = SK_TCP_CLIENT;
        conn->client.serverName = srvName;
        conn->client.port = port;
        conn->fd = s;
        return SK_CONN_OK;
    } else {
        return SK_CONN_ERR;
    }
}

int skConnInitUnixClient(char* err, skConn* conn, char const* path)
{
    int s = skNetUnixConnect(err,path);
    if (s != SK_NET_ERR) {
        char const* pathCopy = strdup(path);
        if (!pathCopy) {
            skFillOOMError(err);
            return SK_CONN_ERR;
        }
        conn->kind = SK_UNIX_CLIENT;
        conn->client.serverName = pathCopy;
        conn->fd = s;
        return SK_CONN_OK;
    } else {
        return SK_CONN_ERR;
    }
}

int skConnInitTcpServer(char* err, skConn* conn, int port, char const* bindAddr, int backlog, int asIpv6)
{
    int s = (asIpv6 ? skNetTcp6Server(err,port,bindAddr,backlog) : skNetTcpServer(err,port,bindAddr,backlog));
    if (s != SK_NET_ERR) {
        char const* bindAddrCopy = 0;
        if (bindAddr && !(bindAddrCopy = strdup(bindAddr))) {
            skFillOOMError(err);
            return SK_CONN_ERR;
        }
        conn->kind = SK_TCP_SERVER;
        conn->tcpServer.bindAddr = bindAddrCopy;
        conn->tcpServer.port = port;
        conn->tcpServer.backlog = backlog;
        conn->fd = s;
        return SK_CONN_OK;
    } else {
        return SK_CONN_ERR;
    }
}

int skConnInitUnixServer(char* err, skConn* conn, char const* path, mode_t perm, int backlog)
{
    int s = skNetUnixServer(err,path,perm,backlog);
    if (s != SK_NET_ERR) {
        char const* pathCopy = strdup(path);
        if (!pathCopy) {
            skFillOOMError(err);
            return SK_CONN_ERR;
        }
        conn->kind = SK_UNIX_SERVER;
        conn->unixServer.path = pathCopy;
        conn->unixServer.perm = perm;
        conn->unixServer.backlog = backlog;
        conn->fd = s;
        return SK_CONN_OK;
    } else {
        return SK_CONN_ERR;
    }
}

int skConnIsValid(skConn* conn)
{
    return conn->fd != -1;
}

int skConnSetNonBlockOption(char* err, skConn* conn, int on)
{
    int s;
    if (on) {
        s = skNetNonBlock(err, conn->fd);
    } else {
        s = skNetBlock(err, conn->fd);
    }
    return s;
}

int skConnSetKeepAliveOption(char* err, skConn* conn, int on, int secs)
{
    skAssert(conn->kind == SK_TCP_CLIENT || conn->kind == SK_TCP_SERVER);
    return skNetKeepAlive(err, conn->fd, secs);
}

int skConnSetSendTimeout(char* err, skConn* conn, sk_ll ms)
{
    return skNetSendTimeout(err, conn->fd, ms);
}

int skConnAccept(char* err, skConn* conn, skConn* client)
{
    skAssert(conn->kind == SK_UNIX_SERVER || conn->kind == SK_TCP_SERVER);
    if (conn->kind == SK_UNIX_SERVER) {
        int s = skNetUnixAccept(err, conn->fd);
        if (s != SK_NET_ERR) {
            char const* pathCopy = strdup(conn->unixServer.path);
            if (!pathCopy) {
                skFillOOMError(err);
                return SK_CONN_ERR;
            }
            client->kind = SK_UNIX_CLIENT;
            client->client.serverName = pathCopy;
            client->fd = s;
            return SK_CONN_OK;
        }
    } else {
        char ip[46];
        int port;
        int s = skNetTcpAccept(err, conn->fd, ip, sizeof(ip), &port);
        if (s != SK_NET_ERR) {
            char const* serverName = strdup(ip);
            if (!serverName) {
                skFillOOMError(err);
                return SK_CONN_ERR;
            }
            client->kind = SK_TCP_CLIENT;
            client->client.serverName = serverName;
            client->client.port = port;
            client->fd = s;
            return SK_CONN_OK;
        }
    }
    return SK_CONN_ERR;
}

int skConnClose(char* err, skConn* conn)
{
    if (skNetClose(err, conn->fd) != SK_NET_ERR) {
        conn->fd = -1;
        return SK_CONN_OK;
    } else {
        return SK_CONN_ERR;
    }
}

int skConnTcpSetNoDelayOption(char* err, skConn* conn, int on)
{
    skAssert(conn->kind == SK_TCP_CLIENT || conn->kind == SK_TCP_SERVER);
    if (on) {
        return skNetEnableTcpNoDelay(err, conn->fd);
    } else {
        return skNetDisableTcpNoDelay(err, conn->fd);
    }
}
