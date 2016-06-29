#ifndef COCOS_SOCKET_H__
#define COCOS_SOCKET_H__

#include <CCPlatformConfig.h>

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <WinSock2.h>
#include <Ws2tcpip.h>
typedef SOCKET cocos_socket_t;
typedef int socklen_t;
#define COCOS_EWOULDBLOCK WSAEWOULDBLOCK
#define COCOS_EAGAIN WSAEWOULDBLOCK
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef int cocos_socket_t;
#define INVALID_SOCKET -1
#define COCOS_EWOULDBLOCK EWOULDBLOCK
#define COCOS_EAGAIN EAGAIN
#endif

cocos_socket_t cocos_socket(int domain, int type, int protocol);

int cocos_connect(cocos_socket_t s, const sockaddr *addr, socklen_t addr_len);

int cocos_setsockopt(cocos_socket_t s, int level, int optname,
	const void *optval, socklen_t optlen);

int cocos_send(cocos_socket_t s, const void *buf, size_t len, int flag);
int cocos_recv(cocos_socket_t s, void *buf, size_t len, int flag);

int cocos_sendto(cocos_socket_t s, const void *buf, size_t len, int flag,
	const sockaddr *addr, socklen_t addr_len);
int cocos_recvfrom(cocos_socket_t s, void *buf, size_t len, int flag,
	sockaddr *addr, socklen_t* addr_len);

int cocos_close(cocos_socket_t s);

void cocos_set_nonblocking(cocos_socket_t s);

int cocos_get_last_error();

/**
 * @retval -1 socket´íÎó
 * @retval 0  ³¬Ê±
 * @retval 1  ¿É¶Á
 */
int cocos_waitfor_readable(cocos_socket_t s, int millisecond);

int cocos_send_n(cocos_socket_t s, const void *buf, size_t len);

#endif
