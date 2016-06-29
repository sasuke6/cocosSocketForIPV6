#include "NetworkHandlerImpl.h" // <WinSock2.h>

#include "NetworkHandler.h"

#include <time.h>
#include <assert.h>
#include <string.h>
#include "Classes/ReconnectHandler.h"

#include "protocol/header.h"
#include "protocol/mh_const.h"
#include "protocol/mh_protocol.h"
#include "protocol/mh_protocol_const.h"	//for kCmdRequestFailed
#include "base/logger.h"
#include "base/InternalEvent.h"
#include "CCLuaEngine.h"
#include "lua/LuaWrapper.h"
#include "util/CmdWatcher.h"
#include "base/EventDispatcher.h"

#include "model/GameSetting.h"

using namespace std::placeholders;

static
void logUnhandledPacket(int cmd, void *buf, int len)
{
	ERROR_MSG("cmd %d, len %d received before default processor is set", cmd, len);
}

NetworkHandler::NetworkHandler()
	: m_seq(1), m_defaultProcessor(logUnhandledPacket), m_impl(NULL)
{
	EventDispatcher::sharedDispatcher().addListener(10005,bind(&NetworkHandler::heartBeatCallback,this,_1,_2,_3));
}

NetworkHandler::~NetworkHandler()
{
	if (m_impl) delete m_impl;
}

bool NetworkHandler::init(const char *serverAddr, unsigned short port)
{
	if(m_impl)
	{
		INFO_MSG("re-init network, %s:%d", serverAddr, port);
		m_impl->reset(serverAddr, port);
	}
	else
	{
		m_impl = new NetworkHandlerImpl(serverAddr, port, this);
	}
	
	if (!m_impl->init())
	{
		return false;
	}
	else 
	{
		return true;
	}
}

void NetworkHandler::handlePacket(uint32_t seq, uint16_t cmd, void *buf, int len)
{
	INFO_MSG("got packet, seq = %d, cmd = %d", seq, cmd);
	PendingTableType::iterator iter = m_pendingRequests.find(seq);
	if(iter == m_pendingRequests.end())
	{
		lua_State *L = cocos2d::CCLuaEngine::defaultEngine()->getLuaStack()->getLuaState();
		bool isLuaHandle =  LuaWrapper::callFuncInLua<bool>(L, "blcxGameCom", "luaRecv","il", (int32_t)cmd, (const char*)buf, (int32_t)len);

		//if (isLuaHandle)
		{
			EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingCompleted, NULL, 0);
			m_pendingLuaRequests.erase(seq);
		}
		m_defaultProcessor(cmd, buf, len);
		return;
	}

	iter->second.processor(cmd, buf, len);
	EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingCompleted, NULL, 0);
	//caution, the callback may have invalidated the iterator
	//we can't use iter here
	m_pendingRequests.erase(seq);
}

void NetworkHandler::tick(float delta)
{
	std::vector<uint32_t> timedOutCalls;
	for(PendingTableType::iterator iter = m_pendingRequests.begin();
		iter != m_pendingRequests.end(); ++iter)
	{
		iter->second.ttl -= delta;
		if(iter->second.ttl <= 0)
			timedOutCalls.push_back(iter->first);
	}

	std::vector<uint32_t> timedOutCallsForLua;
	for(PendingTableType::iterator iter = m_pendingLuaRequests.begin();
		iter != m_pendingLuaRequests.end(); ++iter)
	{
		iter->second.ttl -= delta;
		if(iter->second.ttl <= 0)
			timedOutCallsForLua.push_back(iter->first);
	}

	if ((!timedOutCalls.empty()) || (!timedOutCallsForLua.empty()))
	{
		EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdTimeOut, 0, 0);
	}

	for(size_t i = 0; i < timedOutCalls.size(); ++i)
	{
		EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingCompleted, NULL, 0);
		PendingTableType::iterator iter = m_pendingRequests.find(timedOutCalls[i]);
		assert(iter != m_pendingRequests.end());
		iter->second.failed(kCmdTimedOut, "");
        //the failed(kCmdTimeOut) call may invalidate iter
		m_pendingRequests.erase(timedOutCalls[i]);
	}

	for(size_t i = 0; i < timedOutCallsForLua.size(); ++i)
	{
		EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingCompleted, NULL, 0);
		PendingTableType::iterator iter = m_pendingLuaRequests.find(timedOutCallsForLua[i]);
		assert(iter != m_pendingLuaRequests.end());
		m_pendingLuaRequests.erase(timedOutCallsForLua[i]);
	}

	m_timeElapsed += delta;
	if(m_timeElapsed > kHeartTimeout / 1000 - 5)
	{
		m_timeElapsed = 0.0f;
		ProtoEmpty request;
		send(10005, request);
		//call(10005,request,[=](int code, const std::string &str){
		//	cocos2d::CCLog("===================server=%ld=%s=====",code,str);
		//},
		//	std::bind(&NetworkHandler::heartBeatCallback,this,_1,_2,_3));
	}
}

void NetworkHandler::heartBeatCallback(int cmd,void* buf,int len)
{
	HeartBeatResp resp;
	resp.unpack(buf,len);
	int64_t stime = resp.time;
	//GameSetting::theSetting()->setIncrementTime(stime  - static_cast<int>(time(NULL)));
	GameSetting::theSetting()->setServerTime(stime);
	cocos2d::CCLog("============%d",stime);
}

void NetworkHandler::kickStartSend()
{
	m_impl->kickStartSend();
}

bool NetworkHandler::doReInit()
{
	INFO_MSG("do reinit connection");
	reset();
	return m_impl->init();
}

static bool isLoginCmd(int cmd)
{
	static const int kLoginCmd = 10000;
	return cmd == kLoginCmd;
}

void NetworkHandler::send(uint32_t &seq, uint16_t cmd, const void *buf, int len)
{
	if (!m_impl->connected())
	{
		if(!m_impl->init())
		{
			EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdConnectError, NULL, 0);
			connectionClosed();
			return;
		}
		if (!isLoginCmd(cmd))
		{
			doLogin();
			seq = getNextSeq();
		}
	}
	m_impl->send(seq, cmd, buf, len);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	CmdWatcher::theWatcher()->cmdSent(cmd);
#endif
}

uint32_t NetworkHandler::callForLua(uint16_t cmd, const void *buf, int len)
{
	uint32_t seq = getNextSeq();
	send(seq, cmd, buf, len);

	const float kDefaultTimeout = 6.0f;
	PendingCall c = { kDefaultTimeout, nullptr, nullptr };
	m_pendingLuaRequests[seq] = c;
	EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingBegin, NULL, 0);
	kickStartSend();
	return seq;
}

void NetworkHandler::doLogin()
{
    if(ReconnectHandler::theHandler()) ReconnectHandler::theHandler()->doReconnect();
}

void NetworkHandler::connectionClosed()
{
	PendingTableType tmp;
	tmp.swap(m_pendingRequests);
	PendingTableType::iterator iter = tmp.begin();
	for(; iter != tmp.end(); ++iter)
	{
		EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingCompleted, NULL, 0);
		iter->second.failed(kCmdConnectionReset, "");
	}
	tmp.clear();
	tmp.swap(m_pendingLuaRequests);
	iter = tmp.begin();
	for(; iter != tmp.end(); ++iter)
	{
		EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingCompleted, NULL, 0);
	}
}

void NetworkHandler::pause()
{
    m_impl->pause();
}

void NetworkHandler::resume()
{
    m_impl->resume();
}

void NetworkHandler::reset()
{
    m_impl->reset();
}

bool NetworkHandler::isConnected() const
{
	if(!m_impl)
	{
		return false;
	}

	return m_impl->connected();
}

NetworkHandler &NetworkHandler::sharedNetworkHandler()
{
	static NetworkHandler nh;
	return nh;
}

