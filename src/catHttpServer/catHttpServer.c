/**
 * \file catHttpServer.c
 * \date Jan 19, 2017
 */

#define FD_SETSIZE 4096

#include <servkit/asserts.h>
#include <servkit/connection.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>

// static char errBuf[SK_CONN_ERR_LEN];
// static skConn server;

// static char const* exename = 0;
// static char const* dirnm = 0;
// static char const* hostname = "localhost";
// static int port = 8080;
// static char const* username = 0;

// typedef void(*arg_cbk_t)(char const* val);

// typedef struct
// {
//     char shortId;
//     char const* longId;
//     arg_cbk_t cbk;
//     char const* descr;
//     int args;
// } arg_t;

// static
// void setExeName(char const* val)
// {
//     int i = strlen(val)-1;
//     for (;i >= 0; --i) {
//         if (val[i] == '/') {
//             exename = val+i+1;
//             break;
//         }
//     }
//     exename = (i == -1) ? val : (val+i+1);
// }

// static
// void setHostname(char const* val)
// {
//     hostname = val;
// }

// static
// void setPort(char const* val)
// {
//     char* end = 0;
//     port = (int)strtol(val, &end, 10);
//     if (*end != '\0') {
//         errorAndDie(1, "Not an integer port or has non numeric trailing data.");
//     }
// }

// static
// void setCurrDir(char const* val)
// {
//     dirnm = val;
// }

// static
// void changeCurrDir()
// {
//     if (!dirnm) return;
//     int rv = chdir(dirnm);
//     if (rv != 0) {
//         errorAndDie(1, "%s.", strerror(rv));
//     }
// }

// static
// void setUserName(char const* val)
// {
//     username = val;
// }

// static
// void printUsage(char const* val);

// static
// void changeUserName()
// {
//     if (!username) return;
//     errno = 0;
//     struct passwd* res = getpwnam(username);
//     if (!res) {
//         errorAndDie(1, "%s.", strerror(errno));
//     }
//     if (setgid(res->pw_gid) != 0) {
//         errorAndDie(1, "%s.", strerror(errno));
//     }
//     if (setuid(res->pw_uid) != 0) {
//         errorAndDie(1, "%s.", strerror(errno));
//     }
// }

// static const arg_t opts[] =
// {
//     {'h', "help", printUsage, "This menu.", 0},
//     {'p', "port", setPort, "Port to bind to.", 1},
//     {'n', "host", setHostname, "Hostname to bind to.", 1},
//     {'d', "dir", setCurrDir, "Directory to serve.", 1},
//     {'u', "user", setUserName, "User to serve files as after binding.", 1}
// };

// static
// void usage()
// {
//     int const numOpts = sizeof(opts)/sizeof(opts[0]);
//     printf("Usage for %s\n  %s ", exename, exename);
//     for (int i = 0; i < numOpts; ++i) {
//         if (opts[i].args == 0) {
//             printf("[-%c|--%s] ", opts[i].shortId, opts[i].longId);
//         } else {
//             printf("[-%c|--%s <%s>] ", opts[i].shortId, opts[i].longId, opts[i].longId);
//         }
//     }
//     puts("\nOption descriptions:");
//     for (int i = 0; i < numOpts; ++i) {
//         printf("  %s: %s\n", opts[i].longId, opts[i].descr);
//     }
//     exit(0);
// }

// static
// void printUsage(char const* val)
// {
//     usage();
// }

// static
// void parseArguments(int argc, char const* argv[])
// {
//     setExeName(argv[0]);
//     --argc; // for main executable
//     ++argv;
//     int const numArgs = sizeof(opts)/sizeof(arg_t);
//     for (int arg = 0; arg < argc;) {
//         char const* strArg = argv[arg];
//         int strLen = (int)strlen(strArg);
//         if (strLen < 2) {
//             errorAndDie(1, "Expected an option, with - prefix.");
//         }
//         if (strLen == 2) {
//             if (strArg[0] != '-') errorAndDie(1, "Expected an option, with - prefix.");
//         } else {
//             if (strArg[0] != '-' || strArg[1] != '-') errorAndDie(1, "Expected an option, with -- prefix.");
//         }
//         int i = 0;
//         for (; i < numArgs; ++i) {
//             if (((strLen == 2) && opts[i].shortId == strArg[1]) ||
//                 (strcmp(opts[i].longId, strArg+2) == 0)) {
//                 opts[i].cbk(opts[i].args == 0 ? 0 : argv[arg+1]);
//                 arg += 1+opts[i].args;
//                 break;
//             }
//         }
//         if (i == numArgs) {
//             errorAndDie(1, "Unexpected option: %s.", strArg);
//         }
//     }
// }

// static
// void handleIntr(int sig)
// {
//     exit(0);
// }

// static
// void createServer()
// {
//     dieUnless(skConnInitTcpServer(errBuf, &server, port, hostname, 200, 0) == SK_CONN_OK);
//     // dieUnless(skNetNonBlock(errBuf, server.fd));
//     changeUserName();
//     changeCurrDir();
//     signal(SIGPIPE, SIG_IGN);
//     signal(SIGINT, handleIntr);
// }

// static
// void writeHeaders(skConn* req)
// {
//     char buf[1024];
//     int len;
//     // (void)filename;  /* could use filename to determine file type */

//     strcpy(buf, "HTTP/1.0 200 OK\r\n");
//     len = strlen(buf);
//     if (skConnWrite(errBuf, req, buf, len) < len) {
//         dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
//         return;
//     }
//     strcpy(buf, "Server: catHttpServer/0.1.0\r\n");
//     len = strlen(buf);
//     if (skConnWrite(errBuf, req, buf, len) < len) {
//         dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
//         return;
//     }
//     strcpy(buf, "Content-Type: text/html\r\n\r\n");
//     len = strlen(buf);
//     if (skConnWrite(errBuf, req, buf, len) < len) {
//         dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
//         return;
//     }
//     strcpy(buf, "\r\n");
//     len = strlen(buf);
//     if (skConnWrite(errBuf, req, buf, len) < len) {
//         dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
//         return;
//     }
// }

// static
// void serveFile(skConn* req, int fd)
// {
//     int rv;
//     char buff[4096];
//     dieUnless(skNetNonBlock(errBuf, req->fd) == SK_NET_OK);
//     writeHeaders(req);
//     // dieUnless(skConnSetSendTimeout(errBuf, req, 1) != SK_NET_OK);
//     do {
//         rv = skNetRead(fd, buff, 4096);
//         if (rv == SK_NET_ERR) {
//             strcpy(errBuf, strerror(rv));
//         }
//         dieUnless(rv != SK_NET_ERR);
//         if (skNetWrite(req->fd, buff, rv) < rv) {
//             break;
//         }
//     } while (rv != 0);
// }

// static
// int readLine(skConn const* req, char* buff, int size)
// {
//     char* buffOrig = buff;
//     while (1) {
//         char ch;
//         int rv = recv(req->fd, &ch, 1, 0);
//         if (rv == SK_NET_ERR && errno != EWOULDBLOCK && errno != EAGAIN) {
//             strcpy(errBuf, strerror(errno));
//             dieUnless(0);
//         }
//         if (rv == SK_NET_ERR && errno == EPIPE) {
//             return -1;
//         }
//         *buff++ = ch;
//         if (ch == '\n') {
//             *buff = '\0';
//             break;
//         }
//     }
//     return buff-buffOrig;
// }

// static char* line = 0;
// static int cap = 0;

// void runOld(void)
// {
//   // recreate_server:
//     createServer();
//     if (!line) {
//         cap = 81;
//         line = malloc(cap);
//     }
//     while (1) {
//         skConn req;
//         int fd;
//         int numRead = 0;
//         int thisTime = 0;

//         dieUnless(skConnAccept(errBuf, &server, &req) == SK_CONN_OK);
//         // dieUnless(skNetNonBlock(errBuf, req.fd) != SK_CONN_ERR);

//         while (1) {
//             if (numRead == cap) {
//                 int newcap = 2*cap-1;
//                 char* newline = realloc(line, newcap);
//                 dieUnless(newline != 0);
//                 line = newline;
//                 cap = newcap;
//             }
//             thisTime = readLine(&req, &line[numRead], cap-numRead);
//             if (thisTime == -1) {
//                 goto closeRequestConn;
//             }
//             numRead += thisTime;
//             if ((thisTime == 1 && line[numRead-1] == '\n') ||
//                 (thisTime == 2 && line[numRead-2] == '\r' && line[numRead-1] == '\n')) {
//                 break;
//             }
//         }
//         skDbgTraceF(SK_LVL_SUCC, "Read header from fd=%d\n%s<---", req.fd, line);
//         line[0] = '\0';
//         fd = open("foo.txt", 0);
//         if (fd == -1) {
//             skTraceF(SK_LVL_WARN, "Could not open file.");
//             goto closeRequestConn;
//         }
//         serveFile(&req, fd);
//         close(fd);
//   closeRequestConn:
//         dieUnless(skConnClose(errBuf, &req) == SK_CONN_OK);
//     }
//     free(line);
// }

/*---------------------------------------------------------------------*/

typedef struct
{
    char const* exename;
    char const* dirname;
    char const* hostname;
    char const* username;
    char* line;
    int cap;
    int port;
    char errBuf[SK_CONN_ERR_LEN];
    skConn server;
    int state;
    int errorCode;
    fd_set listenerFds;
    fd_set responseFds;
} catHttpServer;

#define CHS_OK 0
#define CHS_ERR -1

typedef struct
{
    char shortId;
    char const* longId;
    char const* descr;
    int args;
} catHttpServerOption;

static const int HELP_OPT_IDX = 0;

static const catHttpServerOption OPTS[] =
{
    {'h', "help", "This menu.", 0},
    {'p', "port", "Port to bind to.", 1},
    {'n', "host", "Hostname to bind to.", 1},
    {'d', "dir" , "Directory to serve.", 1},
    {'u', "user", "User to serve files as after binding.", 1}
};

static
void usage(catHttpServer* server)
{
    int const numOpts = sizeof(OPTS)/sizeof(OPTS[0]);
    printf("Usage for %s\n  %s ", server->exename, server->exename);
    for (int i = 0; i < numOpts; ++i) {
        if (OPTS[i].args == 0) {
            printf("[-%c|--%s] ", OPTS[i].shortId, OPTS[i].longId);
        } else {
            printf("[-%c|--%s <%s>] ", OPTS[i].shortId, OPTS[i].longId, OPTS[i].longId);
        }
    }
    puts("\nOption descriptions:");
    for (int i = 0; i < numOpts; ++i) {
        printf("  %s: %s\n", OPTS[i].longId, OPTS[i].descr);
    }
}

static
void catHttpServerSetError(catHttpServer* server, int errCode)
{
    server->errorCode = errCode;
    server->state = CHS_ERR;
}

#define catHttpServerSetErrorMsg(srvr, code, fmt, ...) \
  { \
      catHttpServer* srv = (srvr);  \
      catHttpServerSetError(srv, code); \
      sprintf(srv->errBuf, fmt, ##__VA_ARGS__); \
  }

static
void catHttpServerChangeCurrDir(catHttpServer* server)
{
    if (!server->dirname) return;
    int rv = chdir(server->dirname);
    if (rv != 0) {
        catHttpServerSetErrorMsg(server, 1, "%s", strerror(rv));
    }
}

static
void catHttpServerChangeUserName(catHttpServer* server)
{
    struct passwd* res;
    if (!server->username) return;
    errno = 0;
    res = getpwnam(server->username);
    if (!res) {
        catHttpServerSetErrorMsg(server, 1, "%s", strerror(errno));
    }
    if (setgid(res->pw_gid) != 0) {
        catHttpServerSetErrorMsg(server, 1, "%s", strerror(errno));
    }
    if (setuid(res->pw_uid) != 0) {
        catHttpServerSetErrorMsg(server, 1, "%s", strerror(errno));
    }
}

#define warnUnless(...) ((void)(__VA_ARGS__))

static
void catHttpServerWriteHeaders(catHttpServer* server, skConn* req)
{
    char buf[1024];
    int len;
    // (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    len = strlen(buf);
    if (skConnWrite(&server->errBuf[0], req, buf, len) < len) {
        warnUnless(skConnClose(&server->errBuf[0], req) == SK_CONN_OK);
        return;
    }
    strcpy(buf, "Server: catHttpServer/0.1.0\r\n");
    len = strlen(buf);
    if (skConnWrite(&server->errBuf[0], req, buf, len) < len) {
        warnUnless(skConnClose(&server->errBuf[0], req) == SK_CONN_OK);
        return;
    }
    strcpy(buf, "Content-Type: text/html\r\n\r\n");
    len = strlen(buf);
    if (skConnWrite(&server->errBuf[0], req, buf, len) < len) {
        warnUnless(skConnClose(&server->errBuf[0], req) == SK_CONN_OK);
        return;
    }
    strcpy(buf, "\r\n");
    len = strlen(buf);
    if (skConnWrite(&server->errBuf[0], req, buf, len) < len) {
        warnUnless(skConnClose(&server->errBuf[0], req) == SK_CONN_OK);
        return;
    }
}

static
void catHttpServerServeFile(catHttpServer* server, skConn* req, int fd)
{
    int rv;
    char buff[4096];
    catHttpServerWriteHeaders(server, req);
    // warnUnless(skConnSetSendTimeout(errBuf, req, 1) != SK_NET_OK);
    do {
        rv = skNetRead(fd, buff, 4096);
        if (rv == SK_NET_ERR) {
            strcpy(&server->errBuf[0], strerror(rv));
        }
        warnUnless(rv != SK_NET_ERR);
        if (skNetWrite(req->fd, buff, rv) < rv) {
            break;
        }
    } while (rv != 0);
}

static
int catHttpServerReadLine(catHttpServer* server, skConn const* req, char* buff, int size)
{
    char* buffOrig = buff;
    fd_set fds;
    char ch;
    int rv;
    int err;
    FD_ZERO(&fds);
    FD_SET(req->fd, &fds);
    while (1) {
        rv = recv(req->fd, &ch, 1, 0);
        if (rv == SK_NET_ERR) {
            err = errno;
            if (err != EWOULDBLOCK && err != EAGAIN) {
                strcpy(&server->errBuf[0], strerror(err));
                catHttpServerSetError(server, 1);
                return -1;
            } else {
                continue;
            }
        }
        *buff++ = ch;
        if (ch == '\n') {
            *buff = '\0';
            break;
        }
    }
    return buff-buffOrig;
}


static
int catHttpServerOK(catHttpServer const* server)
{
    return server->state == CHS_OK;
}

static
char const* catHttpServerGetError(catHttpServer const* server)
{
    return &(server->errBuf[0]);
}

static
void catHttpServerParseOptions(catHttpServer* server, int argc, char const* argv[])
{
    skAssert(OPTS[HELP_OPT_IDX].shortId == 'h');
    server->exename = server->dirname = server->username = server->line = 0;
    server->hostname = "127.0.0.1";
    server->cap = 0;
    server->port = 8080;
    server->errBuf[0] = '\0';
    server->state = CHS_OK;
    FD_ZERO(&server->listenerFds);
    FD_ZERO(&server->responseFds);

    server->exename = argv[0];
    --argc; ++argv;
    int const numArgs = sizeof(OPTS)/sizeof(catHttpServerOption);
    for (int arg = 0; arg < argc;) {
        char const* strArg = argv[arg];
        int strLen = (int)strlen(strArg);
        if (strLen < 2) {
            catHttpServerSetErrorMsg(server, 1, "Expected an option, with - prefix.");
            return;
        }
        if (strLen == 2) {
            if (strArg[0] != '-') {
                catHttpServerSetErrorMsg(server, 1, "Expected an option, with - prefix.");
                return;
            }
        } else {
            if (strArg[0] != '-' || strArg[1] != '-') {
                catHttpServerSetErrorMsg(server, 1, "Expected an option, with -- prefix.");
                return;
            }
        }
        int i = 0;
        for (; i < numArgs; ++i) {
            if (((strLen == 2) && OPTS[i].shortId == strArg[1]) ||
                (strcmp(OPTS[i].longId, strArg+2) == 0)) {
                if (arg+OPTS[i].args >= argc) {
                    sprintf(server->errBuf, "Insufficient number of arguments for the %s option", OPTS[i].longId);
                    catHttpServerSetError(server, 1);
                    return;
                }
                switch (OPTS[i].shortId) {
                    case 'h':
                        usage(server);
                        catHttpServerSetError(server, 0);
                        break;
                    case 'n':
                        server->hostname = argv[arg+1];
                        break;
                    case 'p':
                    {
                        char* end;
                        server->port = (int)strtol(argv[arg+1], &end, 10);
                        if (end != argv[arg+1] + strlen(argv[arg+1])) {
                            catHttpServerSetErrorMsg(server, 1, "Non numeric port specified: %s", argv[arg+1]);
                            return;
                        }
                    }
                        break;
                    case 'd':
                        server->dirname = argv[arg+1];
                        break;
                    case 'u':
                        server->username = argv[arg+1];
                        break;
                    default:
                        skAssert(0);
                        break;
                }
                arg += 1+OPTS[i].args;
                break;
            }
        }
        if (i == numArgs) {
            catHttpServerSetErrorMsg(server, 1, "Unexpected option: %s", strArg);
            return;
        }
    }
}

static
void catHttpServerLoop(catHttpServer* server)
{
    FD_SET(server->server.fd, &server->listenerFds);
    char* line = 0;
    int cap = 0;
    for (;;) {
        skConn req;
        struct timeval timeout = {0, 50};
      acceptNewConn:
        if (select(1, &server->listenerFds, 0, 0, &timeout) == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ETIMEDOUT) {
                continue;
            }
            if (errno != EINPROGRESS) {
                catHttpServerSetErrorMsg(server, errno, "%s", strerror(errno));
                break;
            }
        }
        // skAssert(FD_ISSET(server->server.fd, &server->listenerFds));
        // if (skConnSetNonBlockOption(&server->errBuf[0], &server->server, 0) == SK_CONN_ERR) {
        //     skTraceF(SK_LVL_WARN, "Failed to set socket non-blocking; fd=%d", server->server.fd);
        //     continue;
        // }
        if (skConnAccept(&server->errBuf[0], &server->server, &req) == SK_CONN_ERR) {
            goto acceptNewConn;
            skTraceF(SK_LVL_WARN, "Failed to accept connection; fd=%d", server->server.fd);
            continue;
        }
        if (!line) {
            cap = 81;
            line = malloc(cap);
        }
        {
            int fd;
            int numRead = 0;
            int thisTime = 0;

            // dieUnless(skConnAccept(errBuf, &server, &req) == SK_CONN_OK);
            if (skNetNonBlock(&server->errBuf[0], req.fd) == SK_CONN_ERR) {
                skTraceF(SK_LVL_WARN, "Could not set non blocking; fd=%d", req.fd);
                goto closeRequestConn;
            }

            while (1) {
                if (numRead == cap) {
                    int newcap = 2*cap-1;
                    char* newline = realloc(line, newcap);
                    if (newline == 0) {
                        catHttpServerSetErrorMsg(server, 1, "Out of memory");
                        return;
                    }
                    line = newline;
                    cap = newcap;
                }
                thisTime = catHttpServerReadLine(server, &req, &line[numRead], cap-numRead);
                if (thisTime == -1) {
                    goto closeRequestConn;
                }
                numRead += thisTime;
                if ((thisTime == 1 && line[numRead-1] == '\n') ||
                    (thisTime == 2 && line[numRead-2] == '\r' && line[numRead-1] == '\n')) {
                    break;
                }
            }
            skDbgTraceF(SK_LVL_SUCC, "Read header from fd=%d\n%s<---", req.fd, line);
            line[0] = '\0';
            fd = open("foo.txt", 0);
            if (fd == -1) {
                skTraceF(SK_LVL_WARN, "Could not open file.");
                goto closeRequestConn;
            }
            catHttpServerServeFile(server, &req, fd);
            close(fd);
      closeRequestConn:
            if (skNetClose(&server->errBuf[0], req.fd) == SK_CONN_ERR) {
                skTraceF(SK_LVL_WARN, "Could not close fd=%d", req.fd);
            }
        }
    }
    FD_CLR(server->server.fd, &server->listenerFds);
    free(line);
}

static
void catHttpServerRun(catHttpServer* server)
{
    if (skConnInitTcpServer(&server->errBuf[0], &server->server, server->port, server->hostname, 200, 0) != SK_CONN_OK) {
        catHttpServerSetError(server, 1);
        return;
    }
    // if (skNetNonBlock(&server->errBuf[0], server->server.fd) == SK_NET_ERR) {
    //     catHttpServerSetError(server, 1);
    //     return;
    // }
    signal(SIGPIPE, SIG_IGN);
    catHttpServerChangeCurrDir(server);
    if (!catHttpServerOK(server)) {
        return;
    }
    catHttpServerChangeUserName(server);

    catHttpServerLoop(server);
}

static
void catHttpServerCloseListener(catHttpServer* server)
{
    if (skConnClose(&server->errBuf[0], &server->server) == SK_CONN_ERR) {
        catHttpServerSetErrorMsg(server, 1, "Error closing listener socket");
    }
}

int main(int argc, char const* argv[])
{
    catHttpServer srv;
    catHttpServerParseOptions(&srv, argc, argv);
    if (!catHttpServerOK(&srv)) {
        skTraceF(SK_LVL_ERR, "%s", catHttpServerGetError(&srv));
        usage(&srv);
        exit(srv.errorCode);
    }
    restartServer:
    catHttpServerRun(&srv);
    if (!catHttpServerOK(&srv)) {
        skTraceF(SK_LVL_WARN, "Server listener fd=%d, went down: %s", srv.server.fd, catHttpServerGetError(&srv));
        catHttpServerCloseListener(&srv);
        if (!catHttpServerOK(&srv)) {
            skTraceF(SK_LVL_ERR, "%s", catHttpServerGetError(&srv));
            exit(1);
        }
        goto restartServer;
    }

    return errno;
}
