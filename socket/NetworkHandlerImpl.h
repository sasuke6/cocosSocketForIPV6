#ifndef MHXJ_NET_NETWORK_HANDLER_IMPL_H__
#define MHXJ_NET_NETWORK_HANDLER_IMPL_H__

#include "cocos_socket.h"	//should be included at the first

#include <functional>

#include "xbuffer.h"
#include <CCObject.h>

class NetworkHandler;

class NetworkHandlerImpl : public cocos2d::CCObject
{
public:
	NetworkHandlerImpl(const char *serverAddr, unsigned short serverPort, NetworkHandler *handler);
	~NetworkHandlerImpl();

	void reset();
	void reset(const char *serverAddr, unsigned short serverPort);
	bool init();
    bool connected();

	void send(uint32_t seq, uint16_t cmd, const void *buf, int len);
	void kickStartSend();

    void pause() { m_paused = true; }
    void resume() { m_paused = false; }

private:
	void initSockaddr(const char *serverAddr, unsigned short serverPort);
	void update(float dt);
    bool m_paused;
	bool m_isIpv6;
	cocos_socket_t m_socket;

	sockaddr_in m_serverAddr4;
	sockaddr_in6 m_serverAddr6;

	XBuffer m_sendBuf;
	XBuffer m_recvBuf;
	NetworkHandler *m_parent;
};

#endif