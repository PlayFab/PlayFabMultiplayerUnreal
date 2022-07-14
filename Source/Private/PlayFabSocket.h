//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Sockets.h"
#include "SocketTypes.h"
#include "PlayFabNetDriver.h"

struct PartyPacket
{
	PartyPacket() = default;

	PartyPacket(uint16 InSourceEndpoint, const TArray<uint8>& InData) :
		SourceEndpoint(InSourceEndpoint),
		Data(InData)
	{}

	uint16 SourceEndpoint = 0;
	TArray<uint8> Data;
};

class FPlayFabSocket : public FSocket
{
PACKAGE_SCOPE:
	static const uint32 SendTimeout;
	
	class FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;
	class FPlayFabSocketSubsystem* SocketSubsystem = nullptr;
	TWeakObjectPtr<UPlayFabNetDriver> NetDriver;
	
	PartyLocalEndpoint* LocalEndpoint = nullptr;

	TCircularQueue<PartyPacket> PendingPackets;

public:
	// FSocket implementation
	FPlayFabSocket(FOnlineSubsystemPlayFab* InOSSPlayFab, const FString& InSocketDescription, const FName& InSocketProtocol);
	virtual ~FPlayFabSocket();

	virtual bool Close() override;

	virtual bool SendTo(const uint8* Data, int32 Count, int32& BytesSent, const FInternetAddr& Destination) override;
	virtual bool Send(const uint8* Data, int32 Count, int32& BytesSent) override;
	virtual bool RecvFrom(uint8* Data, int32 BufferSize, int32& BytesRead, FInternetAddr& Source, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None) override;
 	virtual bool Recv(uint8* Data, int32 BufferSize, int32& BytesRead, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None) override;

	virtual bool HasPendingData(uint32& PendingDataSize) override;
	virtual ESocketConnectionState GetConnectionState() override;

	virtual int32 GetPortNo() override;

	// Unsupported functions
	virtual bool Bind(const FInternetAddr& InAddr) override { return true; }
	virtual bool Connect(const FInternetAddr& InAddr) override { return true; }
	virtual bool Listen(int32 MaxBacklog) override { return true; }
	virtual class FSocket* Accept(const FString& InSocketDescription) override { return nullptr; }
	virtual class FSocket* Accept(FInternetAddr& OutAddr, const FString& InSocketDescription) override { return nullptr; }
	virtual bool Shutdown(ESocketShutdownMode Mode) override { return false; }
	virtual bool Wait(ESocketWaitConditions::Type Condition, FTimespan WaitTime) override { return false; }
	virtual bool WaitForPendingConnection(bool& bHasPendingConnection, const FTimespan& WaitTime) override { return false; }
	virtual bool SetReuseAddr(bool bAllowReuse = true) override { return true; }
	virtual bool SetRecvErr(bool bUseErrorQueue = true) override { return true; }
	virtual bool SetNonBlocking(bool bIsNonBlocking = true) override { return true; }
	virtual bool SetBroadcast(bool bAllowBroadcast = true) override { return false; }
	virtual bool JoinMulticastGroup(const FInternetAddr& GroupAddress) override { return false; }
	virtual bool JoinMulticastGroup(const FInternetAddr& GroupAddress, const FInternetAddr& InterfaceAddress) override { return false; }
	virtual bool LeaveMulticastGroup(const FInternetAddr& GroupAddress) override { return false; }
	virtual bool LeaveMulticastGroup(const FInternetAddr& GroupAddress, const FInternetAddr& InterfaceAddress) override { return false; }
	virtual bool SetMulticastLoopback(bool bLoopback) override { return false; }
	virtual bool SetMulticastTtl(uint8 TimeToPlayFab) override { return false; }
	virtual bool SetMulticastInterface(const FInternetAddr& InterfaceAddress) override { return false; }
	virtual void GetAddress(FInternetAddr& OutAddr) override {}
	virtual bool GetPeerAddress(FInternetAddr& OutAddr) override { return false; }
	virtual bool SetNoDelay(bool bIsNoDelay = true) override { return true; }
	virtual bool SetLinger(bool bShouldLinger = true, int32 Timeout = 0) override { return true; }
	virtual bool SetSendBufferSize(int32 Size, int32& NewSize) override { return true; }
	virtual bool SetReceiveBufferSize(int32 Size, int32& NewSize) override { return true; }


	void AddNewPendingData(uint16 sourceEndpoint, TArray<uint8> NewData);

private:
	// Party instance send message mode
	Party::PartySendMessageOptions PlayFabPartySendMode = Party::PartySendMessageOptions::Default;

	// Party queuing configuration
	Party::PartySendMessageQueuingConfiguration PartyQueueConfiguration = {};
};