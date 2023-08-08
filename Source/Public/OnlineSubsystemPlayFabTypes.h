//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Online/CoreOnline.h"
#include "OnlineSessionSettings.h"
#include "Runtime/Launch/Resources/Version.h"
#include "OnlineSubsystemUtils.h"
#include "PlayFabSDKIncludes.h"

class FPlayFabUser;

class FOnlineSessionInfoPlayFab : public FOnlineSessionInfo
{
public:
	FOnlineSessionInfoPlayFab()
		: SessionId(FUniqueNetIdString::Create(TEXT(""), PLAYFAB_SUBSYSTEM))
	{
	}

	virtual ~FOnlineSessionInfoPlayFab() = default;

	const uint8* GetBytes() const override
	{
		return nullptr;
	}

	int32 GetSize() const override
	{
		return sizeof(FOnlineSessionInfoPlayFab);
	}

	bool IsValid() const override
	{
		return LobbyHandle != nullptr || !ConnectionString.IsEmpty();
	}

	FString ToString() const override
	{
		return SessionName.ToString();
	}

	FString ToDebugString() const override
	{
		return SessionName.ToString();
	}

	virtual const FUniqueNetId& GetSessionId() const override { return *SessionId; }
	void SetSessionId(FString InSessionId) { SessionId = FUniqueNetIdString::Create(InSessionId, PLAYFAB_SUBSYSTEM); }
#if defined(USES_NATIVE_SESSION)
	const FString GetNativeSessionIdString() const { return NativeSessionIdString; }
	void SetNativeSessionIdString(FString InNativeSessionId) { NativeSessionIdString = InNativeSessionId; }
	const FString GetNativePlatform() const { return NativePlatform; }
	void SetNativePlatform(FString InNativePlatform) { NativePlatform = InNativePlatform; }
#endif

	FUniqueNetIdStringRef SessionId;
#if defined(USES_NATIVE_SESSION)
	FString NativeSessionIdString;
	FString NativePlatform;
#endif
	FString ConnectionString;
	PFLobbyHandle LobbyHandle{};
	FName SessionName = NAME_None;
};

const FString INVALID_UNIQUE_NETID = TEXT("");

using FUniqueNetIdPlayFabRef = TSharedRef<const class FUniqueNetIdPlayFab>;
using FUniqueNetIdPlayFabPtr = TSharedPtr<const class FUniqueNetIdPlayFab>;

/**
 * PlayFab Unique Id implementation - EntityId
 */
class FUniqueNetIdPlayFab : public FUniqueNetId
{
protected:
	/** Holds the net id for a player */
	FString UniqueNetId;

	FUniqueNetIdPlayFab() :
		UniqueNetId()
	{
	}

	explicit FUniqueNetIdPlayFab(const FUniqueNetIdPlayFab& Src) :
		UniqueNetId(Src.UniqueNetId)
	{
	}

	explicit FUniqueNetIdPlayFab(const FString& Str) :
		UniqueNetId(Str)
	{
	}

public:

	static FUniqueNetIdPlayFabRef Create(const FString& InUniqueNetIdStr)
	{
		return MakeShareable(new FUniqueNetIdPlayFab(InUniqueNetIdStr));
	}

	static FUniqueNetIdPlayFabRef Create(const FUniqueNetId& InUniqueNetId)
	{
		return MakeShareable(new FUniqueNetIdPlayFab(InUniqueNetId.ToString()));
	}

	virtual FName GetType() const override
	{
		return PLAYFAB_SUBSYSTEM;
	}

	/**
	* Get the raw byte representation of this net id
	* This data is platform dependent and shouldn't be manipulated directly
	*
	* @return byte array of size GetSize()
	*/
	virtual const uint8* GetBytes() const override
	{
		return (uint8*)&UniqueNetId;
	}

	/**
	* Get the size of the id
	*
	* @return size in bytes of the id representation
	*/
	virtual int32 GetSize() const override
	{
		return UniqueNetId.GetAllocatedSize();
	}

	/** Is our structure currently pointing to a valid EntityId? */
	virtual bool IsValid() const override
	{
		return UniqueNetId != INVALID_UNIQUE_NETID;
	}

	/**
	 * Platform specific conversion to string representation of data
	 *
	 * @return data in string form
	 */
	virtual FString ToString() const override
	{
		return UniqueNetId;
	}

	/**
	 * Get a human readable representation of the net id
	 * Shouldn't be used for anything other than logging/debugging
	 *
	 * @return id in string form
	 */
	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("%s"), *OSS_UNIQUEID_REDACT(*this, UniqueNetId));
	}

	/** global static instance of invalid (zero) id */
	static const FUniqueNetIdPlayFabRef& EmptyId()
	{
		static const TSharedRef<const FUniqueNetIdPlayFab> EmptyId = MakeShareable(new FUniqueNetIdPlayFab(INVALID_UNIQUE_NETID));
		return EmptyId;
	}

	bool operator==(FString RawEntityId) const
	{
		return UniqueNetId.Equals(RawEntityId, ESearchCase::IgnoreCase);
	}

	bool operator!=(FString RawEntityId) const
	{
		return !UniqueNetId.Equals(RawEntityId, ESearchCase::IgnoreCase);
	}

	virtual bool Compare(const FUniqueNetId& Other) const override
	{
		return Other.ToString().Equals(this->ToString(), ESearchCase::IgnoreCase);
	}

	virtual ~FUniqueNetIdPlayFab() = default;

	friend FORCEINLINE uint32 GetTypeHash(const FUniqueNetIdPlayFab& Id)
	{
		return GetTypeHashHelper(Id.UniqueNetId);
	}

	friend FArchive& operator<<(FArchive& Ar, FUniqueNetIdPlayFab& UserId)
	{
		return Ar << UserId.UniqueNetId;
	}
};

typedef TSharedPtr<FOnlineSessionInfoPlayFab> FOnlineSessionInfoPlayFabPtr;

namespace EOnlinePlayFabMatchmakingState
{
	enum Type
	{
		None,
		CreatingTicket,
		TicketCreated,
		QueryStatus,	// informational
		Waiting,		// informational
		MatchFound,
		Cancelled,
		Failed,
		JoiningArrangedLobby,
		JoinArrangedLobbyCompleted,
	};
}

class FOnlineMatchmakingTicketInfo
{
public:
	FOnlineMatchmakingTicketInfo()
	{
	}

	FOnlineMatchmakingTicketInfo(
		const FString& InQueueName,
		const FString& InTicketID,
		float InGiveUpAfterTimeInSeconds
	)
		: QueueName(InQueueName)
		, TicketId(InTicketID)
		, GiveUpAfterTimeInSeconds(InGiveUpAfterTimeInSeconds)
		, MatchmakingState(EOnlinePlayFabMatchmakingState::None)
	{
	}

	FString	QueueName;
	FString	TicketId;
	FString	MatchId;
	FString	CancellationReason;
	float	GiveUpAfterTimeInSeconds;

private:
	TSharedPtr<FPlayFabUser> HostUser;

public:
	FName SessionName;
	FOnlineSessionSettings SessionSettings;
	FUniqueNetIdPlayFabPtr SearchingPlayerNetId;
	EOnlinePlayFabMatchmakingState::Type MatchmakingState;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	PFMatchmakingTicketHandle PlayFabMatchTicket;
	const PFMatchmakingMatchDetails* PlayFabMatchmakingDetails;

	TSharedPtr<FPlayFabUser> GetHostUser() const
	{
		return HostUser;
	}

	void SetHostUser(TSharedPtr<FPlayFabUser> InHostUser)
	{
		HostUser = InHostUser;
	}
};

typedef TSharedPtr<FOnlineMatchmakingTicketInfo> FOnlineMatchmakingTicketInfoPtr;
