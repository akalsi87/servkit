/**
 * \file catHttpServer.c
 * \date Jan 19, 2017
 */

#include <servkit/asserts.h>
#include <servkit/connection.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>

static char errBuf[SK_CONN_ERR_LEN];
static skConn server;

static char const* exename = 0;
static char const* dirnm = 0;
static char const* hostname = "localhost";
static int port = 8080;
static char const* username = 0;

#define errorAndDie(code, ...) \
  { \
      skTrace(SK_LVL_ERR, ## __VA_ARGS__); \
      fflush(stderr); \
      exit(code); \
  }

#define dieUnless(expr) \
  if (!(expr)) { errorAndDie(1, "%s.", errBuf); }

typedef void(*arg_cbk_t)(char const* val);

typedef struct
{
    char shortId;
    char const* longId;
    arg_cbk_t cbk;
    char const* descr;
    int args;
} arg_t;

static
void setExeName(char const* val)
{
    int i = strlen(val)-1;
    for (;i >= 0; --i) {
        if (val[i] == '/') {
            exename = val+i+1;
            break;
        }
    }
    exename = (i == -1) ? val : (val+i+1);
}

static
void setHostname(char const* val)
{
    hostname = val;
}

static
void setPort(char const* val)
{
    char* end = 0;
    port = (int)strtol(val, &end, 10);
    if (*end != '\0') {
        errorAndDie(1, "Not an integer port or has non numeric trailing data.");
    }
}

static
void setCurrDir(char const* val)
{
    dirnm = val;
}

static
void changeCurrDir()
{
    if (!dirnm) return;
    int rv = chdir(dirnm);
    if (rv != 0) {
        errorAndDie(1, "%s.", strerror(rv));
    }
}

static
void setUserName(char const* val)
{
    username = val;
}

static
void printUsage(char const* val);

static
void changeUserName()
{
    if (!username) return;
    errno = 0;
    struct passwd* res = getpwnam(username);
    if (!res) {
        errorAndDie(1, "%s.", strerror(errno));
    }
    if (setgid(res->pw_gid) != 0) {
        errorAndDie(1, "%s.", strerror(errno));
    }
    if (setuid(res->pw_uid) != 0) {
        errorAndDie(1, "%s.", strerror(errno));
    }
}

static const arg_t opts[] =
{
    {'h', "help", printUsage, "This menu.", 0},
    {'p', "port", setPort, "Port to bind to.", 1},
    {'n', "host", setHostname, "Hostname to bind to.", 1},
    {'d', "dir", setCurrDir, "Directory to serve.", 1},
    {'u', "user", setUserName, "User to serve files as after binding.", 1}
};

static
void usage()
{
    int const numOpts = sizeof(opts)/sizeof(opts[0]);
    printf("Usage for %s\n  %s ", exename, exename);
    for (int i = 0; i < numOpts; ++i) {
        if (opts[i].args == 0) {
            printf("[-%c|--%s] ", opts[i].shortId, opts[i].longId);
        } else {
            printf("[-%c|--%s <%s>] ", opts[i].shortId, opts[i].longId, opts[i].longId);
        }
    }
    puts("\nOption descriptions:");
    for (int i = 0; i < numOpts; ++i) {
        printf("  %s: %s\n", opts[i].longId, opts[i].descr);
    }
    exit(0);
}

static
void printUsage(char const* val)
{
    usage();
}

static
void parseArguments(int argc, char const* argv[])
{
    setExeName(argv[0]);
    --argc; // for main executable
    ++argv;
    int const numArgs = sizeof(opts)/sizeof(arg_t);
    for (int arg = 0; arg < argc;) {
        char const* strArg = argv[arg];
        int strLen = (int)strlen(strArg);
        if (strLen < 2) {
            errorAndDie(1, "Expected an option, with - prefix.");
        }
        if (strLen == 2) {
            if (strArg[0] != '-') errorAndDie(1, "Expected an option, with - prefix.");
        } else {
            if (strArg[0] != '-' || strArg[1] != '-') errorAndDie(1, "Expected an option, with -- prefix.");
        }
        int i = 0;
        for (; i < numArgs; ++i) {
            if (((strLen == 2) && opts[i].shortId == strArg[1]) ||
                (strcmp(opts[i].longId, strArg+2) == 0)) {
                opts[i].cbk(opts[i].args == 0 ? 0 : argv[arg+1]);
                arg += 1+opts[i].args;
                break;
            }
        }
        if (i == numArgs) {
            errorAndDie(1, "Unexpected option: %s.", strArg);
        }
    }
}

static
void handleIntr(int sig)
{
    exit(0);
}

static
void createServer()
{
    dieUnless(skConnInitTcpServer(errBuf, &server, port, hostname, 200, 0) == SK_CONN_OK);
    // dieUnless(skNetNonBlock(errBuf, server.fd));
    changeUserName();
    changeCurrDir();
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handleIntr);
}

static
void writeHeaders(skConn* req)
{
    char buf[1024];
    int len;
    // (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    len = strlen(buf);
    if (skConnWrite(errBuf, req, buf, len) < len) {
        dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
        return;
    }
    strcpy(buf, "Server: catHttpServer/0.1.0\r\n");
    len = strlen(buf);
    if (skConnWrite(errBuf, req, buf, len) < len) {
        dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
        return;
    }
    strcpy(buf, "Content-Type: text/html\r\n\r\n");
    len = strlen(buf);
    if (skConnWrite(errBuf, req, buf, len) < len) {
        dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
        return;
    }
    strcpy(buf, "\r\n");
    len = strlen(buf);
    if (skConnWrite(errBuf, req, buf, len) < len) {
        dieUnless(skConnClose(errBuf, req) == SK_CONN_OK);
        return;
    }
}

static
void serveFile(skConn* req, int fd)
{
    int rv;
    char buff[4096];
    dieUnless(skNetNonBlock(errBuf, req->fd) == SK_NET_OK);
    writeHeaders(req);
    // dieUnless(skConnSetSendTimeout(errBuf, req, 1) != SK_NET_OK);
    do {
        rv = skNetRead(fd, buff, 4096);
        if (rv == SK_NET_ERR) {
            strcpy(errBuf, strerror(rv));
        }
        dieUnless(rv != SK_NET_ERR);
        if (skNetWrite(req->fd, buff, rv) < rv) {
            break;
        }
    } while (rv != 0);
}

static
int readLine(skConn const* req, char* buff, int size)
{
    char* buffOrig = buff;
    while (1) {
        char ch;
        int rv = recv(req->fd, &ch, 1, 0);
        if (rv == SK_NET_ERR && errno != EWOULDBLOCK && errno != EAGAIN) {
            strcpy(errBuf, strerror(errno));
            dieUnless(0);
        }
        if (rv == SK_NET_ERR && errno == EPIPE) {
            return -1;
        }
        *buff++ = ch;
        if (ch == '\n') {
            *buff = '\0';
            break;
        }
    }
    return buff-buffOrig;
}

static char* line = 0;
static int cap = 0;

void run(void)
{
  // recreate_server:
    createServer();
    if (!line) {
        cap = 81;
        line = malloc(cap);
    }
    while (1) {
        skConn req;
        int fd;
        int numRead = 0;
        int thisTime = 0;

        dieUnless(skConnAccept(errBuf, &server, &req) == SK_CONN_OK);
        // dieUnless(skNetNonBlock(errBuf, req.fd) != SK_CONN_ERR);

        while (1) {
            if (numRead == cap) {
                int newcap = 2*cap-1;
                char* newline = realloc(line, newcap);
                dieUnless(newline != 0);
                line = newline;
                cap = newcap;
            }
            thisTime = readLine(&req, &line[numRead], cap-numRead);
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
        serveFile(&req, fd);
        close(fd);
  closeRequestConn:
        dieUnless(skConnClose(errBuf, &req) == SK_CONN_OK);
    }
    free(line);
}

int main(int argc, char const* argv[])
{
    parseArguments(argc, argv);
    run();
}
