#include "cocos_socket.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include <sys/time.h>
#include <fcntl.h>
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#pragma comment(lib, "ws2_32")

struct WSAStartupInit
{
	WSAStartupInit() { WSAData data; WSAStartup(MAKEWORD(2, 2), &data);}
	~WSAStartupInit() { WSACleanup(); }
};

static WSAStartupInit initObj;

char *buf_cast(void *buf)
{
	return static_cast<char *>(buf);
}

const char *buf_cast(const void *buf)
{
	return static_cast<const char *>(buf);
}
#else
#define buf_cast(x) x
#endif

const sockaddr *sa_cast(const sockaddr_in *sa)
{
	return reinterpret_cast<const sockaddr *>(sa);
}


cocos_socket_t cocos_socket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int cocos_connect(cocos_socket_t s, const sockaddr *addr, socklen_t addr_len)
{
	return connect(s, addr, addr_len);
}

int cocos_send(cocos_socket_t s, const void *buf, size_t len, int flag)
{
	return send(s, buf_cast(buf), len, flag);
}

int cocos_sendto(cocos_socket_t s, const void *buf, size_t len, int flag, const sockaddr *addr, socklen_t addr_len)
{
	return sendto(s, buf_cast(buf), len, flag, addr, addr_len);
}

int cocos_recv(cocos_socket_t s, void *buf, size_t len, int flag)
{
	return recv(s, buf_cast(buf), len, flag);
}

int cocos_recvfrom(cocos_socket_t s, void *buf, size_t len, int flag, sockaddr *addr, socklen_t* addr_len)
{
	return recvfrom(s, buf_cast(buf), len, flag, addr, addr_len);
}

int cocos_close(cocos_socket_t s)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	return closesocket(s);
#else
	return close(s);
#endif
}

int cocos_waitfor_readable(cocos_socket_t s, int millisecond)
{
	fd_set readable;
	FD_ZERO(&readable);
	FD_SET(s, &readable);

	timeval timeout;
	timeout.tv_sec = millisecond / 1000;
	timeout.tv_usec = (millisecond % 1000) * 1000;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	int rtn = select(1, &readable, NULL, NULL, &timeout);
	if(rtn == 0)
		return 0;
	else if(rtn == SOCKET_ERROR)
		return -1;
	return 1;
#else
	return select(s + 1, &readable, NULL, NULL, &timeout);
#endif
}

void cocos_set_nonblocking(cocos_socket_t s)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	unsigned long yes = 1;
	ioctlsocket(s, FIONBIO, &yes);
#else
	int flag = fcntl(s, F_GETFL);
	if((flag & O_NONBLOCK) == 0)
		fcntl(s, F_SETFL, O_NONBLOCK | flag);
#endif
}

int cocos_send_n(cocos_socket_t s, const void *buf, size_t len)
{
	size_t remain = len;
	const char *start = static_cast<const char *>(buf);
	while(len > 0)
	{
		int rtn = cocos_send(s, start, remain, 0);
		if(rtn < 0)
		{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
			if(WSAGetLastError() == WSAEWOULDBLOCK)
				continue;
#else
			if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
				continue;
#endif
			return -1;
		}
		remain -= rtn;
		start += rtn;
	}

	return len;
}

int cocos_get_last_error()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	return WSAGetLastError();
#else
	return errno;
#endif
}
