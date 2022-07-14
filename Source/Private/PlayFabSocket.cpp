//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "PlayFabSocket.h"
#include "PlayFabSocketSubsystem.h"
#include "OnlineSubsystemPlayFab.h"

const uint32 FPlayFabSocket::SendTimeout = 500;

FPlayFabSocket::FPlayFabSocket(FOnlineSubsystemPlayFab* InOSSPlayFab, const FString& InSocketDescription, const FName& InSocketProtocol) :
	FSocket(SOCKTYPE_Datagram, InSocketDescription, InSocketProtocol),
	OSSPlayFab(InOSSPlayFab),
	SocketSubsystem(static_cast<FPlayFabSocketSubsystem*>(ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM))),
	LocalEndpoint(OSSPlayFab->LocalEndpoint),
	PendingPackets(2048)
{
	PartyQueueConfiguration.priority = Party::c_maxSendMessageQueuingPriority;
	PartyQueueConfiguration.identityForCancelFilters = static_cast<uint32>(reinterpret_cast<uintptr_t>(this));
	PartyQueueConfiguration.timeoutInMilliseconds = SendTimeout;
}

FPlayFabSocket::~FPlayFabSocket()
{
	PendingPackets.Empty();
	Close();
}

bool FPlayFabSocket::Close()
{
	return true;
}

bool FPlayFabSocket::SendTo(const uint8* Data, int32 Count, int32& BytesSent, const FInternetAddr& Destination)
{
	BytesSent = 0;

	if (OSSPlayFab == nullptr)
	{
		UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocket::SendTo failed, OSSPlayFab was null"));
		return false;
	}

	if (SocketSubsystem == nullptr)
	{
		UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocket::SendTo failed, SocketSubsystem was null"));
		return false;
	}

	if (LocalEndpoint == nullptr)
	{
		UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocket::SendTo failed, LocalEnpoint was null"));
		SocketSubsystem->LastSocketError = SE_ENOTCONN;
		return false;
	}

	uint32 EndpointId;
	Destination.GetIp(EndpointId);
	PartyEndpoint* RemoteEndpoint = OSSPlayFab->GetPartyEndpoint(EndpointId);
	if (RemoteEndpoint == nullptr)
	{
		// Might be disconnected peer
		UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocket::SendTo failed, no endpoint for %s"), *Destination.ToString(true));
		SocketSubsystem->LastSocketError = SE_ENOTCONN;
		return false;
	}

	if (RemoteEndpoint == LocalEndpoint)
	{
		SocketSubsystem->LastSocketError = SE_NO_ERROR;
		return false;
	}

	int TargetEndPointCount = 1;
	Party::PartyEndpoint* TargetEndpoints[1] = { RemoteEndpoint };

	uint32 DataBufferCount = 1;
	Party::PartyDataBuffer DataBuffer[1] = { { Data, static_cast<uint32_t>(Count) } };

	PartyError Err = LocalEndpoint->SendMessage(
		TargetEndPointCount,		// endpoint count; 0 = broadcast
		TargetEndpoints,			// endpoint list
		PlayFabPartySendMode,		// send message options
		&PartyQueueConfiguration,	// configuration
		DataBufferCount,			// buffer count
		DataBuffer,					// buffer
		nullptr						// async identifier
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG(LogSockets, Warning, TEXT("FPlayFabSocket::Send: Failed so send message: %s"), *GetPartyErrorMessage(Err));

		SocketSubsystem->LastSocketError = SE_ECONNRESET;
		return false;
	}
	else
	{
#if OSS_PLAYFAB_VERBOSE_PACKET_LEVEL_LOGGING
		UE_LOG(LogSockets, Warning, TEXT("FPlayFabSocket::SendTo %d bytes: %d"), EndpointId, Count);
#endif

		BytesSent = Count;
		SocketSubsystem->LastSocketError = SE_NO_ERROR;
		return true;
	}

	return false;
}

bool FPlayFabSocket::Send(const uint8* Data, int32 Count, int32& BytesSent)
{
	// Not supported
	BytesSent = 0;
	ensure(false);
	return false;
}

bool FPlayFabSocket::RecvFrom(uint8* Data, int32 BufferSize, int32& BytesRead, FInternetAddr& Source, ESocketReceiveFlags::Type Flags /*= ESocketReceiveFlags::None*/)
{
	if (BufferSize == 0)
	{
		SocketSubsystem->LastSocketError = SE_NO_ERROR;
		return false;
	}

	PartyPacket CurrentPacket;
	if (!PendingPackets.Dequeue(CurrentPacket))
	{
		// No endpoint associated?
		BytesRead = 0;
		SocketSubsystem->LastSocketError = SE_NO_ERROR;
		return false;
	}

	Source.SetIp(CurrentPacket.SourceEndpoint);
	Source.SetPort(CurrentPacket.SourceEndpoint);
	BytesRead = CurrentPacket.Data.Num();
	FMemory::Memcpy(Data, CurrentPacket.Data.GetData(), BytesRead);

#if OSS_PLAYFAB_VERBOSE_PACKET_LEVEL_LOGGING
	UE_LOG(LogSockets, Warning, TEXT("FPlayFabSocket::RecvFrom bytes: %d UniqueID: %d"), BytesRead, CurrentPacket.SourceEndpoint);
#endif

	return true;
}

bool FPlayFabSocket::Recv(uint8* Data, int32 BufferSize, int32& BytesRead, ESocketReceiveFlags::Type Flags /*= ESocketReceiveFlags::None*/)
{
	// Not supported
	BytesRead = 0;
	return false;
}

bool FPlayFabSocket::HasPendingData(uint32& PendingDataSize)
{
	PendingDataSize = 0;

	const PartyPacket* NextPacket = PendingPackets.Peek();
	if (NextPacket)
	{
		PendingDataSize = NextPacket->Data.Num();
	}

	return PendingDataSize > 0;
}

ESocketConnectionState FPlayFabSocket::GetConnectionState()
{
	return LocalEndpoint ? SCS_Connected : SCS_NotConnected;
}

int32 FPlayFabSocket::GetPortNo()
{
	uint16 EndpointId = 0;

	if (LocalEndpoint)
	{
		LocalEndpoint->GetUniqueIdentifier(&EndpointId);
	}

	return EndpointId;
}

void FPlayFabSocket::AddNewPendingData(uint16 SourceEndpoint, TArray<uint8> NewData)
{
	if (NewData.Num() != 0)
	{
		// Remove oldest packet if we are full up
		if (PendingPackets.IsFull())
		{
			PendingPackets.Dequeue();
		}

		PendingPackets.Enqueue(PartyPacket(SourceEndpoint, NewData));
	}
}