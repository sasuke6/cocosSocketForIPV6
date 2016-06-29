#ifndef MHXJ_NET_NETWORK_HANDLER_H__
#define MHXJ_NET_NETWORK_HANDLER_H__


#include <string>
#include <stdint.h>
#include <functional>
#include <unordered_map>
#include <msgpack.hpp>	//for msgpack::sbuffer, msgpack::packer

class NetworkHandlerImpl;

const int kCmdConnectionReset = -1;
const int kCmdTimedOut = -2;

class NetworkHandler
{
public:
	~NetworkHandler();
	//socket, connect
	bool init(const char *serverIp, unsigned short port);

	typedef std::function<void (uint16_t cmd, void *buf, int len)> ProcessCallback;
	typedef std::function<void (int code, const std::string &reason)> FailedCallback;

	template<class T>
	uint32_t call(uint16_t cmd, const T &msg, FailedCallback failed, ProcessCallback pc);

	template<class T>
	uint32_t send(uint16_t cmd, const T &msg);

	uint32_t callForLua(uint16_t cmd, const void *buf, int len);

	void setDefaultProcessor(ProcessCallback processor) { m_defaultProcessor = processor; }

	static NetworkHandler &sharedNetworkHandler();
    void pause();
    void resume();
    void reset();

	bool doReInit();
	bool isConnected() const;
private:
	NetworkHandler();
	void heartBeatCallback(int cmd,void* buf,int len);
	friend class NetworkHandlerImpl;
	void handlePacket(uint32_t seq, uint16_t cmd, void *buf, int len);
	void tick(float delta);
	void connectionClosed();
    
    void doLogin();
    
	uint32_t getNextSeq()
	{
		++m_seq;
		if(m_seq == 0)
			m_seq = 1;
		return m_seq;
	}

	void send(uint32_t &seq, uint16_t cmd, const void *buf, int len);

	void kickStartSend();
	uint32_t m_seq;

	struct PendingCall
	{
		float ttl;
		ProcessCallback processor;
		FailedCallback failed;
	};

	typedef std::unordered_map<uint32_t, PendingCall> PendingTableType;
	PendingTableType m_pendingRequests;
	PendingTableType m_pendingLuaRequests;
	ProcessCallback m_defaultProcessor;
	NetworkHandlerImpl *m_impl;

	int m_loginReplyId;
    float m_timeElapsed;
};

#include "NetworkHandler.inl"

#endif