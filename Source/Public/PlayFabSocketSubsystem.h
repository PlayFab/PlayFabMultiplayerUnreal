//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Containers/Ticker.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "UObject/WeakObjectPtr.h"
#include "SocketTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"
namespace Party {
	struct PartyEndpointMessageReceivedStateChange;
}
class FOnlineSubsystemPlayFab;

#ifndef PLAYFAB_SOCKET_SUBSYSTEM
#define PLAYFAB_SOCKET_SUBSYSTEM FName(TEXT("PlayFabSockets"))
#endif

class FPlayFabSocket;
class UPlayFabNetDriver;

class FPlayFabSocketSubsystem : public ISocketSubsystem
{
public:

	FPlayFabSocketSubsystem();

	// SocketSubsystem Interface
	virtual bool Init(FString& Error) override;
	virtual void Shutdown() override;
	virtual class FSocket* CreateSocket(const FName& SocketType, const FString& SocketDescription, const FName& ProtocolType) override;
	virtual void DestroySocket(class FSocket* Socket) override;
	virtual FAddressInfoResult GetAddressInfo(const TCHAR* HostName, const TCHAR* ServiceName = nullptr,
		EAddressInfoFlags QueryFlags = EAddressInfoFlags::Default,
		const FName ProtocolTypeName = NAME_None,
		ESocketType SocketType = ESocketType::SOCKTYPE_Unknown) override;
	virtual TSharedPtr<FInternetAddr> GetAddressFromString(const FString& IPAddress) override;
	virtual TSharedRef<FInternetAddr> CreateInternetAddr() override;
	virtual const TCHAR* GetSocketAPIName() const override;
	virtual bool GetLocalAdapterAddresses(TArray<TSharedPtr<FInternetAddr>>& OutAddresses) override;
	virtual TSharedRef<FInternetAddr> GetLocalBindAddr(FOutputDevice& Out) override;
	virtual TSharedRef<FInternetAddr> GetLocalHostAddr(FOutputDevice& Out, bool& bCanBindAll) override;
	
	virtual ESocketErrors GetLastErrorCode() override { return (ESocketErrors)LastSocketError; }
	virtual ESocketErrors TranslateErrorCode(int32 Code) override { return (ESocketErrors)Code; }
	virtual bool GetHostName(FString& HostName) override { return false; }
	virtual bool HasNetworkDevice() override { return true; }
	virtual bool IsSocketWaitSupported() const override { return false; }
	virtual bool RequiresChatDataBeSeparate() override { return false; }
	virtual bool RequiresEncryptedPackets() override { return false; }

	void LinkNetDriver(UPlayFabNetDriver* InNetDriver);
	UPlayFabNetDriver* GetLinkedNetDriver() const
	{
		return NetDriver.Get();
	}

PACKAGE_SCOPE:

	// Last error set by the socket subsystem or one of its sockets
	int32 LastSocketError = 0;

	void RegisterDelegates(FOnlineSubsystemPlayFab* InOSSPlayFab);

	FPlayFabSocket* GetSocket();
	void AddSocket(FPlayFabSocket* NewSocket);
	void RemoveSocket(FPlayFabSocket* Socket);
	void CleanUpActiveSockets();

	// Singleton helpers
	static FPlayFabSocketSubsystem* Create();
	static void Destroy();

	void OnEndpointMessageReceived(const Party::PartyEndpointMessageReceivedStateChange* Change);

protected:
	// Single instantiation of this subsystem 
	static FPlayFabSocketSubsystem* SocketSingleton;

	// Active connection bookkeeping
	TArray<FPlayFabSocket*> ActiveSockets;

	FDelegateHandle OnEndpointMessageReceivedDelegateHandle;

	FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;

	TWeakObjectPtr<UPlayFabNetDriver> NetDriver;
};

 // Create the socket subsystem for the given platform service
FName CreatePlayFabSocketSubsystem();

 // Tear down the socket subsystem for the given platform service
void DestroyPlayFabSocketSubsystem();