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
#include <pwd.h>

static char errBuf[SK_CONN_ERR_LEN];
static skConn server;
static skConn request;

static int fd = 0;
static char const* hostname = "localhost";
static int port = 37000;
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
} arg_t;

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
    int rv = chdir(val);
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
    {'p', "port", setPort},
    {'h', "host", setHostname},
    {'d', "dir", setCurrDir},
    {'u', "user", setUserName}
};

static
void parseArguments(int argc, char const* argv[])
{
    --argc; // for main executable
    ++argv;
    if (argc % 2 != 0) {
        errorAndDie(1, "Expected pairs of options and values.");
    }
    int const numArgs = sizeof(opts)/sizeof(arg_t);
    for (int arg = 0; arg < argc; arg += 2) {
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
                opts[i].cbk(argv[arg+1]);
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
}

static
void acceptConn()
{
    dieUnless(skConnAccept(errBuf, &server, &request) == SK_CONN_OK);
}

static
void writeHeaders()
{
    char buf[1024];
    // (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    write(request.fd, buf, strlen(buf));
    strcpy(buf, "Server: catHttpServer/0.1.0\r\n");
    write(request.fd, buf, strlen(buf));
    strcpy(buf, "Content-Type: text/html\r\n\r\n");
    write(request.fd, buf, strlen(buf));
}

static
void serveFileByChar()
{
    writeHeaders();
    int rv;
    char ch[2];
    do {
        rv = skNetRead(fd, ch, 1);
        if (rv == SK_NET_ERR) {
            strcpy(errBuf, strerror(rv));
        }
        dieUnless(rv != SK_NET_ERR);
        dieUnless(write(request.fd, ch, 1) != SK_CONN_ERR);
    } while (rv != 0);
}

// static
// void serveFileByLine()
// {

// }

static
void serveFile()
{
    // if (fd == 0) {
        serveFileByChar();
    // } else {
        // serveFileByLine();
    // }
}

void run(void)
{
    createServer();
    acceptConn();
    serveFile();
}

int main(int argc, char const* argv[])
{
    parseArguments(argc, argv);
    run();
}
