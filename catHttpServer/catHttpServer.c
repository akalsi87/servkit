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
void createServer()
{
    dieUnless(skConnInitTcpServer(errBuf, &server, port, hostname, 1, 0) == SK_CONN_OK);
    changeUserName();
    changeCurrDir();
}

static
void writeHeaders(skConn const* req)
{
    char buf[1024];
    // (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    write(req->fd, buf, strlen(buf));
    strcpy(buf, "Server: catHttpServer/0.1.0\r\n");
    write(req->fd, buf, strlen(buf));
    strcpy(buf, "Content-Type: text/html\r\n\r\n");
    write(req->fd, buf, strlen(buf));
}

static
void serveFile(skConn const* req, int fd)
{
    writeHeaders(req);
    int rv;
    char buff[4096];
    do {
        rv = skNetRead(fd, buff, 4096);
        if (rv == SK_NET_ERR) {
            strcpy(errBuf, strerror(rv));
        }
        dieUnless(rv != SK_NET_ERR);
        dieUnless(write(req->fd, buff, rv) != SK_CONN_ERR);
    } while (rv != 0);
}

void run(void)
{
    createServer();
    while (1) {
        skConn req;
        int fd;
        dieUnless(skConnAccept(errBuf, &server, &req) == SK_CONN_OK);
        fd = open("foo.txt", 0);
        serveFile(&req, fd);
        close(fd);
        dieUnless(skConnClose(errBuf, &req) == SK_CONN_OK);
    }
}

int main(int argc, char const* argv[])
{
    parseArguments(argc, argv);
    run();
}
