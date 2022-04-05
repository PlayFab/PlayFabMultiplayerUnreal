//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IPAddress.h"
#include "OnlineSubsystemPlayFabPackage.h"

class FInternetAddrPlayFab : public FInternetAddr
{
PACKAGE_SCOPE:
	uint32 EndpointId = 0;
	int32 Port = 0;

	FInternetAddrPlayFab(const FInternetAddrPlayFab& Src);

public:
	FInternetAddrPlayFab() = default;

	explicit FInternetAddrPlayFab(uint32 InUniqueId);

	// FInternetAddr
	virtual TArray<uint8> GetRawIp() const override;
	virtual void SetRawIp(const TArray<uint8>& RawAddr) override;
	virtual void SetIp(uint32 InAddr) override;
	virtual void SetIp(const TCHAR* InAddr, bool& bIsValid) override;
	virtual void GetIp(uint32& OutAddr) const override;
	virtual void SetPort(int32 InPort) override;
	virtual void GetPort(int32& OutPort) const override;
	virtual int32 GetPort() const override;
	virtual void SetAnyAddress() override { /*Not used*/ }
	virtual void SetBroadcastAddress() override { /*Not used*/ }
	virtual void SetLoopbackAddress() override { /*Not used*/ }
	virtual FName GetProtocolType() const override;

	FString ToString(bool bAppendPort) const override;

	virtual bool operator==(const FInternetAddr& Other) const override;
	bool operator!=(const FInternetAddrPlayFab& Other) const;
	virtual uint32 GetTypeHash() const override;
	uint32 GetConstTypeHash() const;
	uint32 GetTypeHash(const FInternetAddrPlayFab& A);
	friend uint32 GetTypeHash(const FInternetAddrPlayFab& A)
	{
		return A.GetConstTypeHash();
	}
	virtual bool IsValid() const override;
	virtual TSharedRef<FInternetAddr> Clone() const override;
};