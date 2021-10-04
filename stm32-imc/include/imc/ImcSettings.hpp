#pragma once

#include <cstdint>

namespace DynaSoft
{

struct ImcSettings
{
	std::uint32_t slaveHandshakeIntervalUs = 100 * 1000;
	std::uint32_t slaveKeepAliveIntervalUs = 100 * 1000;
	std::uint32_t slaveAckTimeoutUs = 300 * 1000;

	std::uint32_t masterCommunicationTimeoutUs = 300 * 1000;
};

}
