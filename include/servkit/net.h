/**
 * \file net.h
 * \date Jan 18, 2017
 */

#ifndef _SERVKIT_NET_H_
#define _SERVKIT_NET_H_

#include <servkit/config.h>

#include <sys/types.h>

#define SK_NET_OK 0
#define SK_NET_ERR -1
#define SK_NET_ERR_LEN 256

SK_API
/*!
 *
 */
int skNetTcpConnect(char* err, char const* addr, int port);

SK_API
/*!
 *
 */
int skNetTcpNonBlockConnect(char* err, char const* addr, int port);

SK_API
/*!
 *
 */
int skNetTcpNonBlockBindConnect(char* err, char* addr, int port, char* sourceAddr);

SK_API
/*!
 *
 */
int skNetTcpNonBlockBestEffortBindConnect(char* err, char* addr, int port, char* sourceAddr);

SK_API
/*!
 *
 */
int skNetUnixConnect(char* err, char const* path);

SK_API
/*!
 *
 */
int skNetUnixNonBlockConnect(char* err, char* path);

SK_API
/*!
 *
 */
int skNetRead(int fd, char* buf, int count);

SK_API
/*!
 *
 */
int skNetResolve(char* err, char* host, char* ipbuf, size_t ipbuf_len);

SK_API
/*!
 *
 */
int skNetResolveIP(char* err, char* host, char* ipbuf, size_t ipbuf_len);

SK_API
/*!
 *
 */
int skNetTcpServer(char* err, int port, char const* bindAddr, int backlog);

SK_API
/*!
 *
 */
int skNetTcp6Server(char* err, int port, char const* bindAddr, int backlog);

SK_API
/*!
 *
 */
int skNetUnixServer(char* err, char const* path, mode_t perm, int backlog);

SK_API
/*!
 *
 */
int skNetTcpAccept(char* err, int srvrSock, char* ip, size_t ipLen, int* port);

SK_API
/*!
 *
 */
int skNetUnixAccept(char* err, int srvrSock);

SK_API
/*!
 *
 */
int skNetWrite(int fd, char const* buf, int count);

SK_API
/*!
 *
 */
int skNetNonBlock(char* err, int fd);

SK_API
/*!
 *
 */
int skNetBlock(char* err, int fd);

SK_API
/*!
 *
 */
int skNetEnableTcpNoDelay(char* err, int fd);

SK_API
/*!
 *
 */
int skNetDisableTcpNoDelay(char* err, int fd);

SK_API
/*!
 *
 */
int skNetTcpKeepAlive(char* err, int fd);

SK_API
/*!
 *
 */
int skNetSendTimeout(char* err, int fd, sk_ll secs);

SK_API
/*!
 *
 */
int skNetPeerToString(int fd, char* ip, size_t ipLen, int* port);

SK_API
/*!
 *
 */
int skNetKeepAlive(char* err, int fd, int interval);

SK_API
/*!
 *
 */
int skNetSockName(int fd, char* ip, size_t ipLen, int* port);

SK_API
/*!
 *
 */
int skNetFormatAddr(char* fmt, size_t fmt_len, char* ip, int port);

SK_API
/*!
 *
 */
int skNetFormatPeer(int fd, char* fmt, size_t fmt_len);

SK_API
/*!
 *
 */
int skNetFormatSock(int fd, char* fmt, size_t fmt_len);

SK_API
/*!
 *
 */
int skNetClose(char* err, int fd);


#endif/*_SERVKIT_NET_H_*/
