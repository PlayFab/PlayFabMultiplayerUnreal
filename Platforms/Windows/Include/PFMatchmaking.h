#pragma once

#include <PFEntityKey.h>

#ifndef __cplusplus
#error "This header requires C++"
#endif // end if not defined __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif

typedef const struct PFMatchmakingTicket* PFMatchmakingTicketHandle;

#pragma pack(push, 8)

/// <summary>
/// The possible states for a matchmaking ticket.
/// </summary>
enum class PFMatchmakingTicketStatus : uint32_t
{
    /// <summary>
    /// The matchmaking ticket is being created.
    /// </summary>
    Creating = 0,

    /// <summary>
    /// The matchmaking ticket is being joined.
    /// </summary>
    Joining = 1,

    /// <summary>
    /// The matchmaking ticket is waiting for all remote users specified in the <c>membersToMatchWith</c> field of its
    /// configuration to join the ticket via <see cref="PFMultiplayerJoinMatchmakingTicketFromId" />.
    /// </summary>
    WaitingForPlayers = 2,

    /// <summary>
    /// The matchmaking ticket is waiting for a match to be found.
    /// </summary>
    WaitingForMatch = 3,

    /// <summary>
    /// The matchmaking ticket has found a match.
    /// </summary>
    Matched = 4,

    /// <summary>
    /// The matchmaking ticket has been canceled.
    /// </summary>
    Canceled = 5,

    /// <summary>
    /// The matchmaking ticket failed to find a match.
    /// </summary>
    Failed = 6,
};

/// <summary>
/// The types of state changes that can occur.
/// </summary>
enum class PFMatchmakingStateChangeType : uint32_t
{
    /// <summary>
    /// A matchmaking ticket status has changed.
    /// </summary>
    /// <remarks>
    /// The PFMatchmakingStateChange object should be cast to a
    /// <see cref="PFMatchmakingTicketStatusChangedStateChange" /> object for more information.
    /// </remarks>
    TicketStatusChanged = 0,

    /// <summary>
    /// A matchmaking ticket has completed.
    /// </summary>
    /// <remarks>
    /// The PFMatchmakingStateChange object should be cast to a <see cref="PFMatchmakingTicketCompletedStateChange" />
    /// object for more information.
    /// </remarks>
    TicketCompleted = 1,
};

/// <summary>
/// A generic, base structure representation of an event or change in state.
/// </summary>
/// <remarks>
/// PFMatchmakingStateChange structures are reported by
/// <see cref="PFMultiplayerStartProcessingMatchmakingStateChanges()" /> for the title to handle and then promptly pass
/// back via the <see cref="PFMultiplayerFinishProcessingMatchmakingStateChanges()" /> method.
/// <para>
/// The <c>stateChangeType</c> field indicates which kind of state change occurred, and this base structure should then
/// be cast to a more specific derived structure to retrieve additional event-specific information.
/// </para>
/// </remarks>
/// <seealso cref="PFMultiplayerStartProcessingMatchmakingStateChanges" />
/// <seealso cref="PFMultiplayerFinishProcessingMatchmakingStateChanges" />
struct PFMatchmakingStateChange
{
    /// <summary>
    /// The specific type of the state change represented.
    /// </summary>
    /// <remarks>
    /// Use this field to determine which corresponding derived structure is represented by this
    /// PFMatchmakingStateChange structure header.
    /// </remarks>
    PFMatchmakingStateChangeType stateChangeType;
};

/// <summary>
/// Information specific to the <em>TicketStatusChanged</em> type of state change.
/// </summary>
struct PFMatchmakingTicketStatusChangedStateChange : PFMatchmakingStateChange
{
    /// <summary>
    /// The matchmaking ticket whose status changed.
    /// </summary>
    /// <remarks>
    /// The new ticket status can be retrieved via <see cref="PFMatchmakingTicketGetStatus" />.
    /// </remarks>
    _Notnull_ PFMatchmakingTicketHandle ticket;
};

/// <summary>
/// Information specific to the <em>TicketCompleted</em> type of state change.
/// </summary>
struct PFMatchmakingTicketCompletedStateChange : PFMatchmakingStateChange
{
    /// <summary>
    /// An error code indicating whether the ticket succeeded or, if it failed, why it failed.
    /// </summary>
    HRESULT result;

    /// <summary>
    /// The matchmaking ticket that completed.
    /// </summary>
    _Notnull_ PFMatchmakingTicketHandle ticket;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;
};

/// <summary>
/// Defines the configuration for a matchmaking ticket.
/// </summary>
struct PFMatchmakingTicketConfiguration
{
    /// <summary>
    /// How long to attempt matchmaking the ticket, in seconds.
    /// </summary>
    uint32_t timeoutInSeconds;

    /// <summary>
    /// The ID of a match queue.
    /// </summary>
    _Null_terminated_ const char * queueName;

    /// <summary>
    /// The number of other specific users expected to join the ticket.
    /// </summary>
    uint32_t membersToMatchWithCount;

    /// <summary>
    /// The PlayFab Entity Keys of other specific users expected to join the ticket.
    /// </summary>
    /// <remarks>
    /// This field specifies the number of other specific users expected to join the ticket. Typically this list
    /// represents a group of friends or fireteam that are looking for a match together. A match can't be found until
    /// all specified users join the ticket via <see cref="PFMultiplayerJoinMatchmakingTicketFromId" />.
    /// </remarks>
    _Field_size_(membersToMatchWithCount) const PFEntityKey * membersToMatchWith;
};

/// <summary>
/// A member of a match result.
/// </summary>
struct PFMatchmakingMatchMember
{
    /// <summary>
    /// The Entity Key for this match member.
    /// </summary>
    PFEntityKey entityKey;

    /// <summary>
    /// The team ID assigned to this match member.
    /// </summary>
    /// <remarks>
    /// May be empty if the matchmaking queue doesn't use team rules.
    /// </remarks>
    _Null_terminated_ const char * teamId;

    /// <summary>
    /// The attributes for this user in serialized JSON format.
    /// </summary>
    _Null_terminated_ const char * attributes;
};

/// <summary>
/// The resulting match information found by a completed ticket.
/// </summary>
struct PFMatchmakingMatchDetails
{
    /// <summary>
    /// The ID of the match.
    /// </summary>
    _Null_terminated_ const char * matchId;

    /// <summary>
    /// The users that have been matched together.
    /// </summary>
    const PFMatchmakingMatchMember * members;

    /// <summary>
    /// The count of members that have been matched together.
    /// </summary>
    uint32_t memberCount;

    /// <summary>
    /// Preferred regions for the match, sorted from most to least preferred.
    /// </summary>
    const char * const * regionPreferences;

    /// <summary>
    /// The count of preferred regions for the match.
    /// </summary>
    uint32_t regionPreferenceCount;

    /// <summary>
    /// The lobby arrangement string associated with the match.
    /// </summary>
    /// <remarks>
    /// This connection string can optionally be used with <see cref="PFMultiplayerJoinArrangedLobby()" /> to join a
    /// lobby associated with this match result. The lobby is not created until a user attempts to join it.
    /// </remarks>
    _Null_terminated_ const char * lobbyArrangementString;
};

#pragma pack(pop)

/// <summary>
/// Retrieves an array of all matchmaking state changes to process since the last such call.
/// </summary>
/// <remarks>
/// This method provides the Matchmaking library an opportunity to synchronize state with remote devices or services,
/// and retrieves a list of all changes currently available for the title since the last call to this method. The title
/// should use the provided array of 0 or more changes to update its own state or UI, and then call
/// <see cref="PFMultiplayerFinishProcessingMatchmakingStateChanges()" /> with them in a timely manner.
/// <para>
/// Matchmaking library state exposed by the library can change during this call, so you must be thread-safe in your use
/// of it. PFMultiplayerStartProcessingMatchmakingStateChanges() should be called frequently-- at least once per
/// graphics frame. It's designed to execute and return quickly such that it can be called on your main UI thread with
/// negligible impact. For best results, you should also minimize the time you spend handling state changes before
/// calling PFMultiplayerFinishProcessingMatchmakingStateChanges().
/// </para>
/// <para>
/// Each state change returned by PFMultiplayerStartProcessingMatchmakingStateChanges() must be returned to
/// PFMultiplayerFinishProcessingMatchmakingStateChanges() exactly once, but may be returned out of order and may be
/// interleaved with state changes from other calls to PFMultiplayerStartProcessingMatchmakingStateChanges(). Any
/// resources associated with a specific state change are guaranteed to stay valid until the state change is returned to
/// PFMultiplayerFinishProcessingMatchmakingStateChanges().
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="stateChangeCount">
/// The output number of PFMatchmakingStateChange entries for the title to handle in the
/// <paramref name="stateChanges" /> array.
/// </param>
/// <param name="stateChanges">
/// A library-allocated output array of all PFMatchmakingStateChange entries for the title to handle and then pass to
/// <see cref="PFMultiplayerFinishProcessingMatchmakingStateChanges()" />.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
/// <seealso cref="PFMatchmakingStateChange" />
/// <seealso cref="PFMultiplayerFinishProcessingMatchmakingStateChanges" />
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerStartProcessingMatchmakingStateChanges(
    PFMultiplayerHandle handle,
    _Out_ uint32_t* stateChangeCount,
    _Outptr_result_buffer_(*stateChangeCount) const PFMatchmakingStateChange * const ** stateChanges
    ) noexcept;

/// <summary>
/// Returns an array of matchmaking state changes that were being processed.
/// </summary>
/// <remarks>
/// This method informs the Matchmaking library that the state changes reported by a previous call to
/// <see cref="PFMultiplayerStartProcessingMatchmakingStateChanges()" /> have now been handled by the title, so their
/// associated resources can be reclaimed. You may call PFMultiplayerFinishProcessingMatchmakingStateChanges() with any
/// number of state changes. Each state change returned by PFMultiplayerStartProcessingMatchmakingStateChanges() must be
/// returned to PFMultiplayerFinishProcessingMatchmakingStateChanges() exactly once, but may be returned out of order
/// and may be interleaved with state changes from other calls to PFMultiplayerStartProcessingMatchmakingStateChanges().
/// <para>
/// Any resources associated with a specific state change are guaranteed to stay valid until the state change is
/// returned to PFMultiplayerFinishProcessingMatchmakingStateChanges().
/// </para>
/// <para>
/// For best results, you should minimize the time you spend handling state changes before calling
/// PFMultiplayerFinishProcessingMatchmakingStateChanges().
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="stateChangeCount">
/// The number of changes, provided in the list specified by <paramref name="stateChanges" />, previously returned by
/// <see cref="PFMultiplayerStartProcessingMatchmakingStateChanges()" /> that have now been handled by the title.
/// </param>
/// <param name="stateChanges">
/// The array of changes previously returned by <see cref="PFMultiplayerStartProcessingMatchmakingStateChanges()" />
/// that have now been handled by the title.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
/// <seealso cref="PFMatchmakingStateChange" />
/// <seealso cref="PFMultiplayerStartProcessingMatchmakingStateChanges" />
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerFinishProcessingMatchmakingStateChanges(
    PFMultiplayerHandle handle,
    uint32_t stateChangeCount,
    _In_reads_(stateChangeCount) const PFMatchmakingStateChange * const * stateChanges
    ) noexcept;

/// <summary>
/// Creates a matchmaking ticket for one or more local users.
/// </summary>
/// <remarks>
/// The library automatically, and asynchronously, will submit all local users on a ticket to the matchmaking service.
/// Each time the ticket status changes, a <see cref="PFMatchmakingTicketStatusChangedStateChange" /> will be provided.
/// The ticket status can be quered at any time via <see cref="PFMatchmakingTicketGetStatus()" />. The ticket
/// immediately starts in the <c>PFMatchmakingTicketStatus::Creating</c> state.
/// <para>
/// When the ticket has completed, a <see cref="PFMatchmakingTicketStatusChangedStateChange" /> will be provided. At
/// that point, a match will have been found or the ticket stopped due to failure. On success, the match that was found
/// can be queried via <see cref="PFMatchmakingTicketGetMatch()" />.
/// </para>
/// <para>
/// All existing tickets in which a local user is a member will be canceled as part of this operation.
/// </para>
/// <para>
/// A match can't be found until all remote users specified in the <c>membersToMatchWith</c> field of the
/// <c>configuration</c> parameter have joined the ticket via <see cref="PFMultiplayerJoinMatchmakingTicketFromId()" />.
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="localUserCount">
/// The count of local users to include in the ticket.
/// </param>
/// <param name="localUsers">
/// The array of local users to include in the ticket.
/// </param>
/// <param name="localUserAttributes">
/// The array of local user attribute strings. There should be one attribute string for each local user. Each attribute
/// string should either be an empty string or a serialized JSON object. For example,
/// <c>{"player_color":"blue","player_role":"tank"}</c>.
/// </param>
/// <param name="configuration">
/// The ticket configuration.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <param name="ticket">
/// The resulting ticket object.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerCreateMatchmakingTicket(
    PFMultiplayerHandle handle,
    uint32_t localUserCount,
    _In_reads_(localUserCount) const PFEntityKey * localUsers,
    _In_reads_(localUserCount) const char * const * localUserAttributes,
    const PFMatchmakingTicketConfiguration * configuration,
    _In_opt_ void * asyncContext,
    _Outptr_ PFMatchmakingTicketHandle* ticket
    ) noexcept;

/// <summary>
/// Joins one or more multiple local users to a matchmaking ticket using a ticket ID and queue name.
/// </summary>
/// <remarks>
/// The library automatically, and asynchronously, will submit all local users to join the ticket on the matchmaking
/// service. Each time the ticket status changes, a <see cref="PFMatchmakingTicketStatusChangedStateChange" /> will be
/// provided. The ticket status can be quered at any time via <see cref="PFMatchmakingTicketGetStatus()" />. The ticket
/// immediately starts in the <c>PFMatchmakingTicketStatus::Joining</c> state.
/// <para>
/// When the ticket has completed, a <see cref="PFMatchmakingTicketStatusChangedStateChange" /> will be provided. At
/// that point, a match will have been found or the ticket stopped due to failure. On success, the match that was found
/// can be queried via <see cref="PFMatchmakingTicketGetMatch()" />.
/// </para>
/// <para>
/// All existing tickets in which a local user is a member will be canceled as part of this operation.
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="localUserCount">
/// The count of local users to join the ticket.
/// </param>
/// <param name="localUsers">
/// The array of local users to join to the ticket.
/// </param>
/// <param name="localUserAttributes">
/// The array of local user attribute strings. There should be one attribute string for each local user. Each attribute
/// string should either be an empty string or a serialized JSON object. For example,
/// <c>{"player_color":"blue","player_role":"tank"}</c>.
/// </param>
/// <param name="ticketId">
/// The ID of the ticket to join.
/// </param>
/// <param name="queueName">
/// The queue to which the ticket belongs.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <param name="ticket">
/// The resulting ticket object.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerJoinMatchmakingTicketFromId(
    PFMultiplayerHandle handle,
    uint32_t localUserCount,
    _In_reads_(localUserCount) const PFEntityKey * localUsers,
    _In_reads_(localUserCount) const char* const* localUserAttributes,
    _Null_terminated_ const char* ticketId,
    _Null_terminated_ const char* queueName,
    _In_opt_ void * asyncContext,
    _Outptr_ PFMatchmakingTicketHandle* ticket
    ) noexcept;

/// <summary>
/// Destroys the matchmaking ticket.
/// </summary>
/// <remarks>
/// This method destroys the matchmaking ticket and reclaims local resources associated with it. If the ticket has not
/// yet completed, a background operation will perform a single best-effort attempt to cancel the ticket with the
/// matchmaking service.
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="ticket">
/// The ticket to destroy.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerDestroyMatchmakingTicket(
    PFMultiplayerHandle handle,
    PFMatchmakingTicketHandle ticket
    ) noexcept;

/// <summary>
/// Provides the matchmaking ticket status.
/// </summary>
/// <param name="status">
/// The matchmaking ticket status.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMatchmakingTicketGetStatus(
    PFMatchmakingTicketHandle ticket,
    _Out_ PFMatchmakingTicketStatus* status
    ) noexcept;

/// <summary>
/// Cancels the ticket.
/// </summary>
/// <remarks>
/// This method queues an asynchronous operation to cancel this matchmaking ticket. On success, a
/// <see cref="PFMatchmakingTicketCompletedStateChange" /> will be provided indicating that the ticket has been
/// canceled.
/// <para>
/// This method does not guarantee the ticket will be canceled. The ticket may complete before the cancelation can be
/// processed, or the cancelation request may fail due to networking or service errors. If the cancelation attempt fails
/// but is retriable, the library will continue to retry the cancelation. Otherwise, a
/// <see cref="PFMatchmakingTicketCompletedStateChange" /> will be provided that indicates the ticket failed.
/// </para>
/// </remarks>
/// <param name="ticket">
/// The handle of the matchmaking ticket.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMatchmakingTicketCancel(
    PFMatchmakingTicketHandle ticket
    ) noexcept;

/// <summary>
/// Provides the ticket ID.
/// </summary>
/// <remarks>
/// The ticket ID will be an empty string if the ticket is in the <c>PFMatchmakingTicketStatus::Creating</c> state or if
/// the ticket is in the <c>PFMatchmakingTicketStatus::Failed</c> state due to failure to submit a ticket to the
/// matchmaking service. The ticket state can be retrieved via <see cref="PFMatchmakingTicketGetStatus" />.
/// </remarks>
/// <param name="ticket">
/// The handle of the matchmaking ticket.
/// </param>
/// <param name="id">
/// The ticket ID.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMatchmakingTicketGetTicketId(
    PFMatchmakingTicketHandle ticket,
    _Outptr_ const char ** id
    ) noexcept;

/// <summary>
/// Provides the match, if one has been found.
/// </summary>
/// <remarks>
/// This method will fail if the ticket isn't in the <c>PFMatchmakingTicketStatus::Matched</c> state. The ticket state
/// can be retrieved via <see cref="PFMatchmakingTicketGetStatus" />.
/// </remarks>
/// <param name="ticket">
/// The handle of the matchmaking ticket.
/// </param>
/// <param name="match">
/// The match that was found.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMatchmakingTicketGetMatch(
    PFMatchmakingTicketHandle ticket,
    _Outptr_ const PFMatchmakingMatchDetails ** match
    ) noexcept;

#ifdef __cplusplus
}
#endif
