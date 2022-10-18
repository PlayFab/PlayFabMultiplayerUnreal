//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

THIRD_PARTY_INCLUDES_START
#if defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_PLAYSTATION
#include <PFEntityKey.h>
#include <PFMultiplayer.h>
#include <PFMatchmaking.h> 
#include <PFLobby.h>
THIRD_PARTY_INCLUDES_END

#include "Runtime/Launch/Resources/Version.h"

/**
 * PlayFab Lobby wrapper
 */
	class FOnlineSessionInfoPlayFab : public FOnlineSessionInfo
{

public:
	/**
	 * Constructor
	 */
	FOnlineSessionInfoPlayFab()
		: SessionId(FUniqueNetIdString::Create(TEXT(""), PLAYFAB_SUBSYSTEM))
		, ConnectionString(FString())
		, LobbyHandle(nullptr)
		, SessionName(FName())
	{
	}

	/**
	 * Constructor
	 *
	 * @param InConnectionString lobby connection string
	 */
	FOnlineSessionInfoPlayFab(FString InConnectionString)
		: SessionId(FUniqueNetIdString::Create(TEXT(""), PLAYFAB_SUBSYSTEM))
		, ConnectionString(InConnectionString)
		, LobbyHandle(nullptr)
		, SessionName(FName())
	{
	}

	/**
	 * Constructor
	 *
	 * @param LobbyId The lobby associated with this session
	 * @param InConnectionString lobby connection string
	 */
	FOnlineSessionInfoPlayFab(FString LobbyId, FString InConnectionString)
		: SessionId(FUniqueNetIdString::Create(LobbyId, PLAYFAB_SUBSYSTEM))
		, ConnectionString(InConnectionString)
		, LobbyHandle(nullptr)
		, SessionName(FName())
	{
	}

	/**
	 * Constructor
	 *
	 * @param LobbyId The lobby associated with this session
	 * @param InLobbyHandle
	 * @param InSessionName
	 */
	FOnlineSessionInfoPlayFab(FString LobbyId, PFLobbyHandle InLobbyHandle, FName InSessionName)
		: SessionId(FUniqueNetIdString::Create(LobbyId, PLAYFAB_SUBSYSTEM))
		, ConnectionString(FString())
		, LobbyHandle(InLobbyHandle)
		, SessionName(InSessionName)
	{
	}

	/**
	 * Constructor
	 *
	 * @param LobbyId The lobby associated with this session
	 * @param InConnectionString lobby connection string
	 * @param InLobbyHandle
	 * @param InSessionName
	 */
	FOnlineSessionInfoPlayFab(FString LobbyId, FString InConnectionString,
							  PFLobbyHandle InLobbyHandle, FName InSessionName)
		: SessionId(FUniqueNetIdString::Create(LobbyId, PLAYFAB_SUBSYSTEM))
		, ConnectionString(InConnectionString)
		, LobbyHandle(InLobbyHandle)
		, SessionName(InSessionName)
	{
	}

	/** Destructor */
	virtual ~FOnlineSessionInfoPlayFab() = default;

	virtual const FUniqueNetId& GetSessionId() const override
	{
		return *SessionId;
	}

	virtual const uint8* GetBytes() const override
	{
		// NOTIMPL
		return NULL;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(FOnlineSessionInfoPlayFab);
	}

	FString GetConnectionString() const
	{
		return ConnectionString;
	}

	PFLobbyHandle GetLobbyHandle() const
	{
		return LobbyHandle;
	}

	FName GetSessionName() const
	{
		return SessionName;
	}

	void SetSessionId(FString InSessionId)
	{
		SessionId = FUniqueNetIdString::Create(InSessionId, PLAYFAB_SUBSYSTEM);
	}

	void SetConnectionString(FString InConnectionString)
	{
		ConnectionString = InConnectionString;
	}

	void SetLobbyHandle(PFLobbyHandle InLobbyHandle)
	{
		LobbyHandle = InLobbyHandle;
	}

	void SetSessionName(FName InSessionName)
	{
		SessionName = InSessionName;
	}

	virtual bool IsValid() const override
	{
		return LobbyHandle != nullptr || !ConnectionString.IsEmpty();
	}

	virtual FString ToString() const override
	{
		return SessionName.ToString();
	}

	virtual FString ToDebugString() const override
	{
		return SessionName.ToString();
	}

	//
	// Platform specific APIs
	//

private:

	/** Placeholder unique id */
	FUniqueNetIdStringRef SessionId;
	FString ConnectionString;
	PFLobbyHandle LobbyHandle;
	FName SessionName;
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

	/** Hidden on purpose */
	FUniqueNetIdPlayFab() :
		UniqueNetId()
	{
	}

	/**
	* Copy Constructor
	*
	* @param Src the id to copy
	*/
	explicit FUniqueNetIdPlayFab(const FUniqueNetIdPlayFab& Src) :
		UniqueNetId(FString(Src.UniqueNetId))
	{
	}

public:

	/**
	 * Constructs this object with the specified net id
	 *
	 * @param String textual representation of an id
	 */
	explicit FUniqueNetIdPlayFab(const FString& Str) :
		UniqueNetId(FString(Str, 0).ToLower())
	{
	}

	static FUniqueNetIdPlayFabRef Create(const FString& InUniqueNetIdStr)
	{
		return MakeShareable(new FUniqueNetIdPlayFab(InUniqueNetIdStr));
	}
	
	static FUniqueNetIdPlayFabRef Create(const FUniqueNetId& InUniqueNetId)
	{
		return StaticCastSharedRef<const FUniqueNetIdPlayFab>(InUniqueNetId.AsShared());
	}

	virtual FName GetType() const override
	{
		#if ENGINE_MAJOR_VERSION >= 5
		#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
			return GDK_SUBSYSTEM;
		#elif defined(OSS_PLAYFAB_SWITCH)
			return SWITCH_SUBSYSTEM;
		#elif defined(OSS_PLAYFAB_WIN64)
			return STEAM_SUBSYSTEM;
		#elif defined(OSS_PLAYFAB_PS4)
			return PS4_SUBSYSTEM;
		#elif defined(OSS_PLAYFAB_PS5)
			return PS5_SUBSYSTEM;
		#endif // OSS_PLAYFAB_WINGDK || OSS_PLAYFAB_XSX || OSS_PLAYFAB_XBOXONEGDK
		UE_LOG_ONLINE(Error, TEXT("Unsupported Platform."));
		return FName();
		#else // ENGINE_MAJOR_VERSION
		return PLAYFAB_SUBSYSTEM;
		#endif // ENGINE_MAJOR_VERSION
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
		return ::GetTypeHash(Id.UniqueNetId);
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
