//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "IPAddressPlayFab.h"
#include "OnlineSubsystemPlayFabPrivate.h"
#include "OnlineSubsystemPlayFabDefines.h"

FInternetAddrPlayFab::FInternetAddrPlayFab(uint32 InUniqueId) :
	EndpointId(InUniqueId),
	Port(InUniqueId)
{
}

FInternetAddrPlayFab::FInternetAddrPlayFab(const FInternetAddrPlayFab& Src) : 
	EndpointId(Src.EndpointId), 
	Port(Src.Port)
{
}

void FInternetAddrPlayFab::SetIp(uint32 InAddr)
{
	EndpointId = InAddr;
}

void FInternetAddrPlayFab::GetIp(uint32& OutAddr) const
{
	OutAddr = EndpointId;
}

void FInternetAddrPlayFab::SetPort(int32 InPort)
{
	Port = InPort;
}

void FInternetAddrPlayFab::GetPort(int32& OutPort) const
{
	OutPort = Port;
}

int32 FInternetAddrPlayFab::GetPort() const
{
	return Port;
}

FName FInternetAddrPlayFab::GetProtocolType() const
{
	return PLAYFAB_SUBSYSTEM;
}

TArray<uint8> FInternetAddrPlayFab::GetRawIp() const
{
	TArray<uint8> RawAddressArray;
	const uint8* RawData = reinterpret_cast<const uint8*>(&EndpointId);
	for (uint32 DataIndex = 0; DataIndex < sizeof(EndpointId); DataIndex++)
	{
		RawAddressArray.Add(RawData[DataIndex]);
	}

	return RawAddressArray;
}

void FInternetAddrPlayFab::SetRawIp(const TArray<uint8>& RawAddr)
{
	check(RawAddr.Num() == sizeof(EndpointId));
	if (RawAddr.Num() != sizeof(EndpointId))
	{
		return;
	}

	FMemory::Memcpy(&EndpointId, RawAddr.GetData(), sizeof(EndpointId));
	Port = EndpointId;
}

void FInternetAddrPlayFab::SetIp(const TCHAR* InAddr, bool& bIsValid)
{
	bIsValid = false;

	if (FCString::Strncmp(InAddr, PLAYFAB_URL_PREFIX, FCString::Strlen(PLAYFAB_URL_PREFIX)) == 0)
	{
		const TCHAR* AddrStart = InAddr + UE_ARRAY_COUNT(PLAYFAB_URL_PREFIX) - 1;
		if (AddrStart)
		{
			const TCHAR* PortStart = FCString::Strrchr(AddrStart, L':');
			if (PortStart == nullptr)
			{
				PortStart = AddrStart + FCString::Strlen(AddrStart) + 1;
			}
			else
			{
				PortStart++;
			}

			Port = FCString::Atoi(PortStart);

			FString PartyIdString = FString(AddrStart, static_cast<int>(static_cast<intptr_t>(PortStart - AddrStart)) - 1);
			if (PartyIdString.Len() > 0)
			{
				EndpointId = FCString::Atoi(*PartyIdString);
				bIsValid = EndpointId != 0;
			}
		}
	}
}

FString FInternetAddrPlayFab::ToString(bool bAppendPort) const
{
	if (bAppendPort)
	{
		return FString::Printf(TEXT("%s%d:%d"), PLAYFAB_URL_PREFIX, EndpointId, Port);
	}
	else
	{
		return FString::Printf(TEXT("%s%d"), PLAYFAB_URL_PREFIX, EndpointId);
	}
}

uint32 FInternetAddrPlayFab::GetTypeHash() const
{
	return GetConstTypeHash();
}

bool FInternetAddrPlayFab::IsValid() const
{
	return EndpointId != 0;
}

TSharedRef<FInternetAddr> FInternetAddrPlayFab::Clone() const
{
	return MakeShareable(new FInternetAddrPlayFab(*this));
}

uint32 FInternetAddrPlayFab::GetConstTypeHash() const
{
	return ::GetTypeHash(ToString(true));
}

bool FInternetAddrPlayFab::operator!=(const FInternetAddrPlayFab& Other) const
{
	return !(FInternetAddrPlayFab::operator==(Other));
}

bool FInternetAddrPlayFab::operator==(const FInternetAddr& Other) const
{
	FInternetAddrPlayFab& PlayFabOther = (FInternetAddrPlayFab&)Other;
	return EndpointId == PlayFabOther.EndpointId;
}