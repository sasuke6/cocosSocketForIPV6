#include "NetworkHandlerImpl.h"

#include <CCScheduler.h>
#include <CCDirector.h>
#include "protocol/header.h"
#include "net/NetworkHandler.h"
#include "base/logger.h"
#include "util/str_util.h"

NetworkHandlerImpl::NetworkHandlerImpl(const char *serverAddr, uint16_t serverPort,
	NetworkHandler *handler)
	: m_socket(INVALID_SOCKET), m_sendBuf(8192), m_recvBuf(8192), m_parent(handler),
    m_paused(false), m_isIpv6(false)
{
	initSockaddr(serverAddr, serverPort);
}

NetworkHandlerImpl::~NetworkHandlerImpl()
{
	if(m_socket != INVALID_SOCKET)
		cocos_close(m_socket);
}

static void exitApp_()
{
	cocos2d::CCDirector::sharedDirector()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	exit(0);
#endif
}
void NetworkHandlerImpl::initSockaddr(const char *serverAddr, unsigned short serverPort)
{
    struct addrinfo *answer, hint;
	memset(&hint, 0, sizeof(hint)); 
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	int ret = getaddrinfo(serverAddr, NULL, &hint, &answer);
	if (ret != 0) { 
		exitApp_();
		return;
	} 
	
    switch (answer->ai_family)
    {
    case AF_UNSPEC: 
        //do something here 
        break; 
    case AF_INET:
        m_isIpv6 = false;

        m_serverAddr4 = *reinterpret_cast<sockaddr_in *>( answer->ai_addr);
        m_serverAddr4.sin_port = htons(serverPort);
        break;
    case AF_INET6: 
        m_isIpv6 = true;

        m_serverAddr6 = *reinterpret_cast<sockaddr_in6 *>( answer->ai_addr);
        m_serverAddr6.sin6_port = htons(serverPort);
        break; 
    } 
	 
	freeaddrinfo(answer);
}

bool NetworkHandlerImpl::init()
{
	assert(m_socket == INVALID_SOCKET);
	if(m_isIpv6)
	{
		m_socket = cocos_socket(AF_INET6, SOCK_STREAM, 0);
	}
	else
	{
		m_socket = cocos_socket(AF_INET, SOCK_STREAM, 0);
	}

	if(m_socket == INVALID_SOCKET) return false;

	int rtn = 0;

	if(m_isIpv6)
	{
		rtn = cocos_connect(m_socket, (struct sockaddr *)&m_serverAddr6,
			sizeof(m_serverAddr6));
	}
	else
	{
		rtn = cocos_connect(m_socket, (struct sockaddr *)&m_serverAddr4,
			sizeof(m_serverAddr4));
	}
	
	if(rtn != 0)
	{
		char str[256];
		xy_snprintf(str, sizeof(str), "connect error: %d", cocos_get_last_error());
		CRITICAL_MSG(str);
		cocos_close(m_socket);
		m_socket = INVALID_SOCKET;
		return false;
	}

	cocos_set_nonblocking(m_socket);
	cocos2d::CCDirector::sharedDirector()->getScheduler()->scheduleUpdateForTarget(this, 1, false);
	return true;
}

bool NetworkHandlerImpl::connected()
{
    return m_socket != INVALID_SOCKET;
}

void NetworkHandlerImpl::kickStartSend()
{
	if(m_socket != INVALID_SOCKET)
	{
		int rtn = m_sendBuf.WriteTo(m_socket);
		if(rtn < 0)
		{
			ERROR_MSG("send error: %d", rtn);
			reset();
		}
	}
	else
	{
		m_parent->connectionClosed();
	}
}

void NetworkHandlerImpl::reset()
{
	m_recvBuf.Clear();
	m_sendBuf.Clear();
	cocos_close(m_socket);
	cocos2d::CCDirector::sharedDirector()->getScheduler()->unscheduleUpdateForTarget(this);
	m_socket = INVALID_SOCKET;

	m_parent->connectionClosed();
}

void NetworkHandlerImpl::reset(const char *serverAddr, unsigned short serverPort)
{
	reset();

	initSockaddr(serverAddr, serverPort);
}

void NetworkHandlerImpl::update(float dt)
{
	if(m_socket == INVALID_SOCKET || m_paused) return;
	kickStartSend();

	if(m_socket == INVALID_SOCKET || m_paused) return;
	int rtn = m_recvBuf.ReadFrom(m_socket);
	if(rtn == 0)
	{
		WARNING_MSG("socket closed by peer!!");
		reset();
		return;
	}
	else if(rtn < 0)
	{
		ERROR_MSG("recv from socket error, %d!!!!", rtn);
		reset();
		return;
	}

	while(m_recvBuf.ReadableBytes() >= sizeof(PacketHeader))
	{
		PacketHeader header;
		memcpy(&header, m_recvBuf.ReadPos(), sizeof(header));
		header.len = ntohs(header.len);
		header.cmd = ntohs(header.cmd);
		header.seq = ntohl(header.seq);
        
        if(m_recvBuf.ReadableBytes() < header.len)
            break;

        //we got a full packet
        m_recvBuf.ConsumeBytes(sizeof(header));
        int bodyLen = header.len + 2 - sizeof(header);
        m_parent->handlePacket(header.seq, header.cmd, m_recvBuf.ReadPos(), bodyLen);
		if (m_recvBuf.ReadableBytes() <= 0) break;
        m_recvBuf.ConsumeBytes(bodyLen);

        if(m_paused)
            break;
	}

	m_parent->tick(dt);
    
}

void NetworkHandlerImpl::send(uint32_t seq, uint16_t cmd, const void *buf, int len)
{
	PacketHeader header;
	header.len = htons(sizeof(header) + len - 2);
	header.cmd = htons(cmd);
	header.seq = htonl(seq);

	m_sendBuf.Append(&header, sizeof(header));
	m_sendBuf.Append(buf, len);
}