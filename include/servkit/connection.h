/**
 * \file connection.h
 * \date Jan 19, 2017
 */

#ifndef _SERVKIT_CONNECTION_H_
#define _SERVKIT_CONNECTION_H_

#include <servkit/net.h>

#define SK_CONN_OK SK_NET_OK
#define SK_CONN_ERR SK_NET_ERR
#define SK_CONN_ERR_LEN SK_NET_ERR_LEN

enum skConnKind
{
    SK_TCP_CLIENT,
    SK_UNIX_CLIENT,
    SK_TCP_SERVER,
    SK_UNIX_SERVER
};

typedef enum skConnKind skConnKind;

struct skConnDataClient
{
    int port;
    char const* serverName;
};

typedef struct skConnDataClient skConnDataClient;

struct skConnDataTcpServer
{
    int port;
    int backlog;
    char const* bindAddr;
};

typedef struct skConnDataTcpServer skConnDataTcpServer;

struct skConnDataUnixServer
{
    mode_t perm;
    int backlog;
    char const* path;
};

typedef struct skConnDataUnixServer skConnDataUnixServer;

struct skConn
{
    int fd;
    union
    {
        skConnDataClient client;
        skConnDataTcpServer tcpServer;
        skConnDataUnixServer unixServer;
    };
    int kind;
    int keepAlive;
    int nonBlock;

};

typedef struct skConn skConn;

SK_API
/*!
 *
 */
int skConnInitTcpClient(char* err, skConn* conn, char const* serverName, int port);

SK_API
/*!
 *
 */
int skConnInitUnixClient(char* err, skConn* conn, char const* path);

SK_API
/*!
 *
 */
int skConnInitTcpServer(char* err, skConn* conn, int port, char const* bindAddr, int backlog, int asIpv6);

SK_API
/*!
 *
 */
int skConnInitUnixServer(char* err, skConn* conn, char const* path, mode_t perm, int backlog);

SK_API
/*!
 *
 */
int skConnIsValid(skConn* conn);

SK_API
/*!
 *
 */
int skConnSetNonBlockOption(char* err, skConn* conn, int on);

SK_API
/*!
 *
 */
int skConnSetKeepAliveOption(char* err, skConn* conn, int on, int secs);

SK_API
/*!
 *
 */
int skConnSetSendTimeout(char* err, skConn* conn, sk_ll ms);

SK_API
/*!
 *
 */
int skConnAccept(char* err, skConn* conn, skConn* client);

SK_API
/*!
 *
 */
int skConnClose(char* err, skConn* conn);

/** Tcp APIs */

SK_API
/*!
 *
 */
int skConnTcpSetNoDelayOption(char* err, skConn* conn, int on);

#endif/*_SERVKIT_CONNECTION_H_*/
