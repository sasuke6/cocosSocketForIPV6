#include <msgpack.hpp>

#include "base/InternalEvent.h"
#include "base/EventDispatcher.h"
#include "base/logger.h"

template <class T>
uint32_t NetworkHandler::call(uint16_t cmd, const T &msg, FailedCallback failed,
	ProcessCallback pc)
{

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	#ifdef _DEBUG
		const float kDefaultTimeout = 6.0f;
	#else
		const float kDefaultTimeout = 6.0f;
	#endif
#else
	const float kDefaultTimeout = 6.0f;
#endif
	
	uint32_t seq = send(cmd, msg);

	INFO_MSG("call cmd = %d, seq = %d", cmd, seq);

	PendingCall c = { kDefaultTimeout, pc, failed };
	m_pendingRequests[seq] = c;
	EventDispatcher::sharedDispatcher().routeEvent(kInternalCmdLoadingBegin, NULL, 0);
	kickStartSend();
	return seq;
}

template <class T>
uint32_t NetworkHandler::send(uint16_t cmd, const T &msg)
{
	msgpack::sbuffer buffer(1024);
	msgpack::packer<msgpack::sbuffer> pk(&buffer);
	msg.pack(&pk);
	uint32_t seq = getNextSeq();
	send(seq, cmd, buffer.data(), buffer.size());

	return seq;
}
