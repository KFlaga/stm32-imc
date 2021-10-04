#pragma once

namespace DynaSoft
{

/// Suspends Uart sending for lifetime of this object.
template<typename Uart>
class UartSendLock
{
public:
	UartSendLock(Uart& u) :
		uart{u}
	{
		u.suspendSend();
	}

	~UartSendLock()
	{
		uart.resumeSend();
	}

private:
	Uart& uart;
};

/// Suspends Uart receiving for lifetime of this object.
template<typename Uart>
class UartReceiveLock
{
public:
	UartReceiveLock(Uart& u) :
		uart{u}
	{
		u.suspendReceive();
	}

	~UartReceiveLock()
	{
		uart.resumeReceive();
	}

private:
	Uart& uart;
};
}
