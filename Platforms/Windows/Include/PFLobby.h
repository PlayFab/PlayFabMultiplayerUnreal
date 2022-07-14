#pragma once

#include <PFEntityKey.h>

#ifndef __cplusplus
#error "This header requires C++"
#endif // end if not defined __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif

typedef const struct PFLobby* PFLobbyHandle;

/// <summary>
/// The minimum allowed value for <see cref="PFLobbyCreateConfiguration::maxMemberCount" /> and
/// <see cref="PFLobbyDataUpdate::maxMemberCount" />.
/// </summary>
constexpr uint32_t PFLobbyMaxMemberCountLowerLimit = 2;

/// <summary>
/// The maximum allowed value for <see cref="PFLobbyCreateConfiguration::maxMemberCount" /> and
/// <see cref="PFLobbyDataUpdate::maxMemberCount" />.
/// </summary>
constexpr uint32_t PFLobbyMaxMemberCountUpperLimit = 128;

/// <summary>
/// The maximum number of concurrent search properties which can be stored for the lobby.
/// </summary>
constexpr uint32_t PFLobbyMaxSearchPropertyCount = 30;

/// <summary>
/// The maximum number of concurrent properties which can be stored for the lobby and which aren't owned by any specific
/// member.
/// </summary>
constexpr uint32_t PFLobbyMaxLobbyPropertyCount = 30;

/// <summary>
/// The maximum number of concurrent properties allowed for each member in the lobby.
/// </summary>
constexpr uint32_t PFLobbyMaxMemberPropertyCount = 30;

/// <summary>
/// The maximum number of search results that client-entity callers may request when performing a
/// <see cref="PFMultiplayerFindLobbies" /> operation.
/// </summary>
constexpr uint32_t PFLobbyClientRequestedSearchResultCountUpperLimit = 50;

/// <summary>
/// A special, pre-defined search key which can be used in the <see cref="PFLobbySearchConfiguration" /> filtering and
/// sorting strings to search for lobbies based on the current number of members in the lobby.
/// </summary>
/// <remarks>
/// Example: "lobby/memberCount lt 5"
/// </remarks>
constexpr char PFLobbyMemberCountSearchKey[] = "lobby/memberCount";

/// <summary>
/// A special, pre-defined search key which can be used in the <see cref="PFLobbySearchConfiguration" /> filtering
/// string to search for lobbies that you are currently a member of.
/// </summary>
/// <remarks>
/// Example: "lobby/amMember eq true"
/// </remarks>
constexpr char PFLobbyAmMemberSearchKey[] = "lobby/amMember";

#pragma pack(push, 8)

/// <summary>
/// The types of state changes that can occur.
/// </summary>
enum class PFLobbyStateChangeType : uint32_t
{
    /// <summary>
    /// The operation started by a previous call to <see cref="PFMultiplayerCreateAndJoinLobby()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyCreateAndJoinLobbyCompletedStateChange" />
    /// object for more information.
    /// </remarks>
    CreateAndJoinLobbyCompleted = 0,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFMultiplayerJoinLobby()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyJoinLobbyCompletedStateChange" /> object for
    /// more information.
    /// </remarks>
    JoinLobbyCompleted = 1,

    /// <summary>
    /// A PlayFab entity was added to a lobby as a member.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyMemberAddedStateChange" /> object for more
    /// information.
    /// </remarks>
    MemberAdded = 2,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFLobbyAddMember()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyAddMemberCompletedStateChange" /> object for
    /// more information.
    /// </remarks>
    AddMemberCompleted = 3,

    /// <summary>
    /// A PlayFab entity was removed from a lobby as a member.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyMemberRemovedStateChange" /> object for more
    /// information.
    /// </remarks>
    MemberRemoved = 4,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFLobbyForceRemoveMember()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyForceRemoveMemberCompletedStateChange" />
    /// object for more information.
    /// </remarks>
    ForceRemoveMemberCompleted = 5,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFLobbyLeave()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyLeaveLobbyCompletedStateChange" /> object
    /// for more information.
    /// </remarks>
    LeaveLobbyCompleted = 6,

    /// <summary>
    /// A lobby was updated.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyUpdatedStateChange" /> object for more
    /// information.
    /// </remarks>
    Updated = 7,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFLobbyPostUpdate()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyPostUpdateCompletedStateChange" /> object
    /// for more information.
    /// <para>
    /// This operation completing only indicates whether the Lobby service has accepted the update or not. The title's
    /// local view of the Lobby state will not reflect this update until a <see cref="PFLobbyUpdatedStateChange" /> is
    /// provided to the title with the updated state.
    /// </para>
    /// </remarks>
    PostUpdateCompleted = 8,

    /// <summary>
    /// The client has started disconnecting from a lobby.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyDisconnectingStateChange" /> object for more
    /// information.
    /// </remarks>
    Disconnecting = 9,

    /// <summary>
    /// The client has disconnected from a lobby.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyDisconnectedStateChange" /> object for more
    /// information.
    /// </remarks>
    Disconnected = 10,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFMultiplayerJoinArrangedLobby()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyJoinArrangedLobbyCompletedStateChange" />
    /// object for more information.
    /// </remarks>
    JoinArrangedLobbyCompleted = 11,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFMultiplayerFindLobbies()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyFindLobbiesCompletedStateChange" /> object
    /// for more information.
    /// </remarks>
    FindLobbiesCompleted = 12,

    /// <summary>
    /// An entity on this client has received an invite to a lobby.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyInviteReceivedStateChange" /> object for
    /// more information.
    /// </remarks>
    InviteReceived = 13,

    /// <summary>
    /// An invite listener's status has changed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbyInviteListenerStatusChangedStateChange" />
    /// object for more information.
    /// </remarks>
    InviteListenerStatusChanged = 14,

    /// <summary>
    /// The operation started by a previous call to <see cref="PFLobbySendInvite()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PFLobbyStateChange object should be cast to a <see cref="PFLobbySendInviteCompletedStateChange" /> object
    /// for more information.
    /// </remarks>
    SendInviteCompleted = 15,
};

/// <summary>
/// Reasons why a member was removed from a lobby.
/// </summary>
enum class PFLobbyMemberRemovedReason : uint32_t
{
    /// <summary>
    /// The local user is being removed because the title called <see cref="PFLobbyLeave()" />.
    /// </summary>
    LocalUserLeftLobby = 0,

    /// <summary>
    /// The local user entity was forcibly removed by the owner.
    /// </summary>
    LocalUserForciblyRemoved = 1,

    /// <summary>
    /// The remote user has been removed from the lobby. It is unspecified whether they left voluntarily or if they were
    /// forcibly removed by the owner.
    /// </summary>
    RemoteUserLeftLobby = 2,
};

/// <summary>
/// Reasons the client no longer has access to the Lobby.
/// </summary>
enum class PFLobbyDisconnectingReason : uint32_t
{
    /// <summary>
    /// The client is no longer connected to the lobby because none of the client's local entities are members.
    /// </summary>
    NoLocalMembers = 0,

    /// <summary>
    /// The client is being disconnected from the lobby because the lobby's server owner has deleted the lobby.
    /// </summary>
    LobbyDeleted = 1,

    /// <summary>
    /// The client is being disconnected from the lobby because the client has lost connection.
    /// </summary>
    ConnectionInterruption = 2,
};

/// <summary>
/// The available policies the lobby service can use to migrate lobby ownership between members.
/// </summary>
enum class PFLobbyOwnerMigrationPolicy : uint32_t
{
    /// <summary>
    /// Once the lobby owner is disconnected, a new owner is chosen at random from the set of connected members.
    /// </summary>
    Automatic = 0,

    /// <summary>
    /// Once the lobby owner is disconnected, any member may elect themselves the new owner.
    /// </summary>
    /// <remarks>
    /// Until a new owner is chosen, <see cref="PFLobbyGetOwner" /> will return a null owner.
    /// </remarks>
    Manual = 1,

    /// <summary>
    /// At any point, any member may elect themselves the owner of the lobby, regardless of the state of the current
    /// owner.
    /// </summary>
    /// <remarks>
    /// If the current owner leaves, <see cref="PFLobbyGetOwner" /> will return a null owner until a new owner elects
    /// themselves.
    /// </remarks>
    None = 2,

    /// <summary>
    /// The server is the owner and owner migration is not possible.
    /// </summary>
    Server = 3,
};

/// <summary>
/// Values representing the lobby's access policy.
/// </summary>
/// <remarks>
/// The access policy controls whether this lobby's connection string is accessible in search queries.
/// </remarks>
enum class PFLobbyAccessPolicy : uint32_t
{
    /// <summary>
    /// The lobby is both visible in queries and any player may join, including invited players.
    /// </summary>
    Public = 0,

    /// <summary>
    /// The lobby and its connection string are queryable by friends of members in this lobby.
    /// </summary>
    /// <remarks>
    /// In some multiplayer social networks, friendship is a unidirectional relationship. One user may "follow" another
    /// or be their friend, but the same is not necessarily true in the reverse direction. This access policy only
    /// grants access when a bidirectional friendship exists. That is, the user querying for the lobby and the user in
    /// the lobby must both be friends with each other.
    /// <para>
    /// When querying for lobbies, users can opt into searching external multiplayer social networks for friendship
    /// relationships as well as the native PlayFab friends list. For example, a user could opt to check their Xbox Live
    /// friends list in addition to the PlayFab friends list. An external multiplayer social relationship can only be
    /// considered if both PlayFab users for this title have linked that mulitplayer social network to their PlayFab
    /// accounts.
    /// </para>
    /// </remarks>
    Friends = 1,

    /// <summary>
    /// The lobby is not visible in queries, and a player must receive an invite to join.
    /// </summary>
    Private = 2,
};

/// <summary>
/// Values representing the state of the lobby's membership lock.
/// </summary>
enum class PFLobbyMembershipLock : uint32_t
{
    /// <summary>
    /// Lobby membership is unlocked. New members will not be prevented from joining.
    /// </summary>
    Unlocked = 0,

    /// <summary>
    /// Lobby membership is locked. New members will be prevented from joining.
    /// </summary>
    Locked = 1,
};

/// <summary>
/// Values representing the current status of an invite listener.
/// </summary>
enum class PFLobbyInviteListenerStatus : uint32_t
{
    /// <summary>
    /// The invite listener has not been started or has been stopped and is not listening for invites.
    /// </summary>
    NotListening = 0,

    /// <summary>
    /// The invite listener has been established and is listening for invites.
    /// </summary>
    Listening = 1,

    /// <summary>
    /// The listening entity was not authorized to establish an invite listener.
    /// </summary>
    /// <remarks>
    /// This status is fatal. The title should clear the invite listener with
    /// <see cref="PFMultiplayerStopListeningForLobbyInvites" />. No invites will be received on the corresponding
    /// listener after this status update.
    /// <para>
    /// Receiving this status likely represents a programming error where an invalid entity has been passed to
    /// <c>PFMultiplayerStopListeningForLobbyInvites</c>.
    /// </para>
    /// </remarks>
    NotAuthorized = 2,
};

/// <summary>
/// The initial configuration data used when creating a lobby.
/// </summary>
struct PFLobbyCreateConfiguration
{
    /// <summary>
    /// The maximum number of members allowed in the new lobby.
    /// </summary>
    /// <remarks>
    /// This value must be at least <c>PFLobbyMaxMemberCountLowerLimit</c> and no more than
    /// <c>PFLobbyMaxMemberCountUpperLimit</c>.
    /// <para>
    /// If a client would violate this limit by calling <see cref="PFMultiplayerJoinLobby()" /> or
    /// <see cref="PFLobbyAddMember" />, the operation will fail asynchronously.
    /// </para>
    /// </remarks>
    _Field_range_(PFLobbyMaxMemberCountLowerLimit, PFLobbyMaxMemberCountUpperLimit) uint32_t maxMemberCount;

    /// <summary>
    /// The owner migration policy for the new lobby.
    /// </summary>
    /// <remarks>
    /// This value cannot be set to <c>PFLobbyOwnerMigrationPolicy::Server</c>.
    /// </remarks>
    PFLobbyOwnerMigrationPolicy ownerMigrationPolicy;

    /// <summary>
    /// The access policy for the new lobby.
    /// </summary>
    PFLobbyAccessPolicy accessPolicy;

    /// <summary>
    /// The number of initial search properties for the new lobby.
    /// </summary>
    _Field_range_(0, PFLobbyMaxSearchPropertyCount) uint32_t searchPropertyCount;

    /// <summary>
    /// The keys of the initial search properties for the new lobby.
    /// </summary>
    /// <remarks>
    /// Search properties are visible to non-members of the lobby as metadata which can be used to filter and sort lobby
    /// search results.
    /// <para>
    /// Search properties must be of the form string_keyN or number_keyN where "N" is a number between 1 and
    /// <c>PFLobbyMaxSearchPropertyCount</c>. e.g. string_key1, number_key14, etc.
    /// <para>
    /// </remarks>
    _Field_size_(searchPropertyCount) const char * const * searchPropertyKeys;

    /// <summary>
    /// The values of the initial search properties for the new lobby.
    /// </summary>
    /// <remarks>
    /// Search properties are visible to non-members of the lobby as metadata which can be used to filter and sort lobby
    /// search results.
    /// </remarks>
    _Field_size_(searchPropertyCount) const char * const * searchPropertyValues;

    /// <summary>
    /// The number of initial lobby properties for the new lobby.
    /// </summary>
    _Field_range_(0, PFLobbyMaxLobbyPropertyCount) uint32_t lobbyPropertyCount;

    /// <summary>
    /// The keys of the initial lobby properties for the new lobby.
    /// </summary>
    /// <remarks>
    /// Lobby properties are only visible to members of the lobby.
    /// </remarks>
    _Field_size_(lobbyPropertyCount) const char * const * lobbyPropertyKeys;

    /// <summary>
    /// The values of the initial lobby properties for the new lobby.
    /// </summary>
    /// <remarks>
    /// Lobby properties are only visible to members of the lobby.
    /// </remarks>
    _Field_size_(lobbyPropertyCount) const char * const * lobbyPropertyValues;
};

/// <summary>
/// The initial configuration data used when joining an arranged lobby.
/// </summary>
struct PFLobbyArrangedJoinConfiguration
{
    /// <summary>
    /// The maximum number of members allowed in the lobby, if joiner is the first member in the lobby.
    /// </summary>
    /// <remarks>
    /// This value must be at least <c>PFLobbyMaxMemberCountLowerLimit</c> and no more than
    /// <c>PFLobbyMaxMemberCountUpperLimit</c>.
    /// <para>
    /// If a client would violate this limit by calling <see cref="PFMultiplayerJoinLobby()" /> or
    /// <see cref="PFLobbyAddMember" />, the operation will fail asynchronously.
    /// </para>
    /// </remarks>
    _Field_range_(PFLobbyMaxMemberCountLowerLimit, PFLobbyMaxMemberCountUpperLimit) uint32_t maxMemberCount;

    /// <summary>
    /// The owner migration policy for the lobby, if the joiner is the first member in the lobby.
    /// </summary>
    /// <remarks>
    /// This value cannot be set to <c>PFLobbyOwnerMigrationPolicy::Server</c>.
    /// </remarks>
    PFLobbyOwnerMigrationPolicy ownerMigrationPolicy;

    /// <summary>
    /// The access policy for the lobby, if the joiner is the first member in the lobby.
    /// </summary>
    PFLobbyAccessPolicy accessPolicy;

    /// <summary>
    /// The number of initial member properties for the joiner of the lobby.
    /// </summary>
    _Field_range_(0, PFLobbyMaxMemberPropertyCount) uint32_t memberPropertyCount;

    /// <summary>
    /// The keys of the initial member properties for the joiner of the lobby.
    /// </summary>
    /// <remarks>
    /// Per-member properties are only visible to members of the lobby.
    /// </remarks>
    _Field_size_(memberPropertyCount) const char * const * memberPropertyKeys;

    /// <summary>
    /// The values of the initial member properties for the joiner of the lobby.
    /// </summary>
    /// <remarks>
    /// Per-member properties are only visible to members of the lobby.
    /// </remarks>
    _Field_size_(memberPropertyCount) const char * const * memberPropertyValues;
};

/// <summary>
/// The initial configuration data used when joining a lobby.
/// </summary>
struct PFLobbyJoinConfiguration
{
    /// <summary>
    /// The number of initial member properties for the joiner of the lobby.
    /// </summary>
    _Field_range_(0, PFLobbyMaxMemberPropertyCount) uint32_t memberPropertyCount;

    /// <summary>
    /// The keys of the initial member properties for the joiner of the lobby.
    /// </summary>
    /// <remarks>
    /// Per-member properties are only visible to members of the lobby.
    /// </remarks>
    _Field_size_(memberPropertyCount) const char * const * memberPropertyKeys;

    /// <summary>
    /// The values of the initial member properties for the joiner of the lobby.
    /// </summary>
    /// <remarks>
    /// Per-member properties are only visible to members of the lobby.
    /// </remarks>
    _Field_size_(memberPropertyCount) const char * const * memberPropertyValues;
};

/// <summary>
/// A request to make an update to the shared portion of the lobby on behalf of a member.
/// </summary>
struct PFLobbyDataUpdate
{
    /// <summary>
    /// An optional, new owner of the lobby.
    /// </summary>
    /// <remarks>
    /// This value can only be updated under one of the following conditions:
    /// ` * The member updating this field is the lobby's current owner
    /// ` * The owner migration policy is <c>PFLobbyOwnerMigrationPolicy::Manual</c> and there is currently no owner
    /// ` * The owner migration policy is <c>PFLobbyOwnerMigrationPolicy::None</c>
    /// </remarks>
    _Maybenull_ const PFEntityKey * newOwner;

    /// <summary>
    /// An optional, updated capacity for the number of members in this lobby.
    /// </summary>
    /// <remarks>
    /// This new value must be greater than than the number of members currently in the lobby and less than
    /// <c>PFLobbyMaxMemberCountUpperLimit</c>.
    /// <para>
    /// This value can only be updated by the current lobby owner.
    /// </para>
    /// </remarks>
    _Maybenull_ const uint32_t * maxMemberCount;

    /// <summary>
    /// An optional, updated access policy for this lobby.
    /// </summary>
    /// <remarks>
    /// This value can only be updated by the current lobby owner.
    /// </remarks>
    _Maybenull_ const PFLobbyAccessPolicy * accessPolicy;

    /// <summary>
    /// An optional update to the membership lock on this lobby.
    /// </summary>
    /// <remarks>
    /// This value can only be updated by the current lobby owner.
    /// </remarks>
    _Maybenull_ const PFLobbyMembershipLock * membershipLock;

    /// <summary>
    /// The number of search properties to update.
    /// </summary>
    /// <remarks>
    /// Only the current lobby owner can update the search properties.
    /// <para>
    /// There may only be <c>PFLobbyMaxSearchPropertyCount</c> concurrent search properties at any given time.
    /// Therefore, at most, twice that many unique properties can be specified in this update if half of those
    /// properties are being deleted.
    /// </para>
    /// <para>
    /// If the property limits are violated, the entire update operation will fail.
    /// </para>
    /// </remarks>
    _Field_range_(0, PFLobbyMaxSearchPropertyCount * 2) uint32_t searchPropertyCount;

    /// <summary>
    /// The keys of the search properties to update.
    /// </summary>
    /// <remarks>
    /// Only the current lobby owner can update the lobby properties.
    /// <para>
    /// Search properties are visible to non-members of the lobby as metadata which can be used to filter and sort lobby
    /// search results.
    /// </para>
    /// <para>
    /// Only the properties specified in this list of keys will be updated. If the key doesn't exist yet, the property
    /// will be created. If the new property value is nullptr, the property will be deleted. Any existing properties
    /// omitted from this list will be left unmodified.
    /// </para>
    /// <para>
    /// Search properties must be of the form string_keyN or number_keyN where "N" is a number between 1 and
    /// <c>PFLobbyMaxSearchPropertyCount</c>. e.g. string_key1, number_key14, etc.
    /// <para>
    /// </remarks>
    _Field_size_(searchPropertyCount) const char * const * searchPropertyKeys;

    /// <summary>
    /// The values of the search properties to update.
    /// </summary>
    /// <remarks>
    /// Only the current lobby owner can update the search properties.
    /// <para>
    /// Search properties are visible to non-members of the lobby as metadata which can be used to filter and sort lobby
    /// search results.
    /// </para>
    /// <para>
    /// To delete a value, provide nullptr as its new value.
    /// </para>
    /// </remarks>
    _Field_size_(searchPropertyCount) const char * const * searchPropertyValues;

    /// <summary>
    /// The number of lobby properties to update.
    /// </summary>
    /// <remarks>
    /// Only the current lobby owner can update the lobby properties.
    /// <para>
    /// There may only be <c>PFLobbyMaxLobbyPropertyCount</c> concurrent lobby properties at any given time. Therefore,
    /// at most, twice that many unique properties can be specified in this update if half of those properties are being
    /// deleted.
    /// </para>
    /// <para>
    /// If the property limits are violated, the entire update operation will fail.
    /// </para>
    /// </remarks>
    _Field_range_(0, PFLobbyMaxLobbyPropertyCount * 2) uint32_t lobbyPropertyCount;

    /// <summary>
    /// The keys of the lobby properties to update.
    /// </summary>
    /// <remarks>
    /// Only the current lobby owner can update the lobby properties.
    /// <para>
    /// Lobby properties are only visible to members of the lobby.
    /// </para>
    /// <para>
    /// Only the properties specified in this list of keys will be updated. If the key doesn't exist yet, the property
    /// will be created. If the new property value is nullptr, the property will be deleted. Any existing properties
    /// omitted from this list will be left unmodified.
    /// </para>
    /// </remarks>
    _Field_size_(lobbyPropertyCount) const char * const * lobbyPropertyKeys;

    /// <summary>
    /// The values of the lobby properties to update.
    /// </summary>
    /// <remarks>
    /// Only the current lobby owner can update the lobby properties.
    /// <para>
    /// Lobby properties are only visible to members of the lobby.
    /// </para>
    /// <para>
    /// To delete a value, provide nullptr as its new value.
    /// </para>
    /// </remarks>
    _Field_size_(lobbyPropertyCount) const char * const * lobbyPropertyValues;
};

/// <summary>
/// A request to make an update to the member-owned portion of the lobby document.
/// </summary>
struct PFLobbyMemberDataUpdate
{
    /// <summary>
    /// The number of member properties to update for the updating member.
    /// </summary>
    /// <remarks>
    /// There may only be <c>PFLobbyMaxMemberPropertyCount</c> concurrent properties at any given time per-member.
    /// Therefore, at most, twice that many unique properties can be specified in this update if half of those
    /// properties are being deleted.
    /// <para>
    /// If the property limits are violated, the entire update operation will fail.
    /// </para>
    /// </remarks>
    _Field_range_(0, PFLobbyMaxMemberPropertyCount * 2) uint32_t memberPropertyCount;

    /// <summary>
    /// The keys of the member properties to update for the updating member.
    /// </summary>
    /// <remarks>
    /// Per-member properties are only visible to members of the lobby.
    /// <para>
    /// Only the properties specified in this list of keys will be updated. If the key doesn't exist yet, the property
    /// will be created. If the new property value is nullptr, the property will be deleted. Any existing properties
    /// omitted from this list will be left unmodified.
    /// </para>
    /// </remarks>
    _Field_size_(memberPropertyCount) const char * const * memberPropertyKeys;

    /// <summary>
    /// The values of the member properties to update for the updating member.
    /// </summary>
    /// <remarks>
    /// Per-member properties are only visible to members of the lobby.
    /// <para>
    /// To delete a value, provide nullptr as its new value.
    /// </para>
    /// </remarks>
    _Field_size_(memberPropertyCount) const char * const * memberPropertyValues;
};

/// <summary>
/// A collection of hints about an update which has been successfully applied to the lobby on behalf of a member.
/// </summary>
struct PFLobbyMemberUpdateSummary
{
    /// <summary>
    /// The member which performed the update
    /// </summary>
    PFEntityKey member;

    /// <summary>
    /// A flag indicating whether the member's connection status has updated.
    /// </summary>
    bool connectionStatusUpdated;

    /// <summary>
    /// The number of member properties which have been updated for <c>member</c>.
    /// </summary>
    _Field_range_(0, PFLobbyMaxMemberPropertyCount * 2) uint32_t updatedMemberPropertyCount;

    /// <summary>
    /// The keys of the member properties which have been updated for <c>member</c>.
    /// </summary>
    _Field_size_(updatedMemberPropertyCount) const char * const * updatedMemberPropertyKeys;
};

/// <summary>
/// The filter structure used to limit lobby search results to only those lobbies owned by the player's friends.
/// </summary>
/// <remarks>
/// Regardless of which external friend lists are included when constructing this filter, friends from the PlayFab
/// friends list will always be included.
/// </remarks>
struct PFLobbySearchFriendsFilter
{
    /// <summary>
    /// A flag which includes the player's Steam friends list if their PlayFab account is linked to their Steam account.
    /// </summary>
    bool includeSteamFriends;

    /// <summary>
    /// A flag which includes the player's Facebook friends list if their PlayFab account is linked to their Facebook
    /// account.
    /// </summary>
    bool includeFacebookFriends;

    /// <summary>
    /// An Xbox Live token that, when provided, includes the player's Xbox Live friends list if their PlayFab account is
    /// linked to their Xbox Live account.
    /// </summary>
    /// <remarks>
    /// To retrieve this token, make a POST request to "https://playfabapi.com" with an empty request body using one of
    /// the GetTokenAndSignature APIs provided by Xbox Live.
    /// <para>
    /// GetTokenAndSignature APIs are provided natively as part of the Microsoft Game Core Development Kit (GDK). On all
    /// other platforms, these APIs are provided via the Xbox Authentication Library API (XAL).
    /// </para>
    /// </remarks>
    _Maybenull_ _Null_terminated_ const char * includeXboxFriendsToken;
};

/// <summary>
/// The configuration structure used to specify how a <see cref="PFMultiplayerFindLobbies" /> operation should be
/// performed.
/// </summary>
struct PFLobbySearchConfiguration
{
    /// <summary>
    /// A filter that, when provided, will constrain the lobby search operation to only those owned by the members of
    /// that player's various friend lists.
    /// </summary>
    /// <remarks>
    /// If omitted, the search operation will search all available lobbies.
    /// </remarks>
    _Maybenull_ const PFLobbySearchFriendsFilter * friendsFilter;

    /// <summary>
    /// The query string used to filter which lobbies are returned in the search results.
    /// </summary>
    /// <remarks>
    /// This string is formatted in an OData-like filtering syntax.
    /// <para>
    /// Only the following operators are supported: "and" (logical and), "eq" (equal), "ne" (not equals), "ge" (greater
    /// than or equal), "gt" (greater than), "le" (less than or equal), and "lt" (less than).
    /// </para>
    /// <para>
    /// The left-hand side of each OData logical expression should be either a search property key (e.g. string_key1,
    /// number_key3, etc) or one of the pre-defined search keys (<c>PFLobbyMemberCountSearchKey</c> or
    /// <c>PFLobbyAmMemberSearchKey</c>).
    /// </para>
    /// <para>
    /// The left-hand side of each OData logical expression should be a search property key.
    /// </para>
    /// <para>
    /// This string cannot exceed 500 characters.
    /// </para>
    /// <para>
    /// Example: "string_key1 eq 'CaptureTheFlag' and number_key10 gt 50 and lobby/memberCount lt 5"
    /// </para>
    /// </remarks>
    _Maybenull_ _Null_terminated_ const char * filterString;

    /// <summary>
    /// The query string used to sort the lobbies returned in the search results.
    /// </summary>
    /// <remarks>
    /// This string is formatted in an OData-like order-by syntax: a comma-separated list of search property keys with
    /// an optional specifier to sort in either ascending or descending order.
    /// <para>
    /// To specify ascending order, use the "asc" operator after the associated search property key. To specify
    /// descending order, use the "desc" operator after the associated search property key.
    /// </para>
    /// <para>
    /// Additionally, a special sorting moniker, distance, is supported to enable sorting by closest distance from some
    /// numeric value. For example, "distance{number_key10=5} asc" will sort the results so that lobbies who have their
    /// "number_key10" search property closer to the value "5" will return earlier in the search results.
    /// </para>
    /// <para>
    /// This string cannot exceed 100 characters.
    /// </para>
    /// <para>
    /// Example: "string_key1 asc,lobby/memberCount desc"
    /// </para>
    /// </remarks>
    _Maybenull_ _Null_terminated_ const char * sortString;

    /// <summary>
    /// An optional value which, when specified, will limit the number of results provided in the completion response.
    /// </summary>
    /// <remarks>
    /// This value may only be specified when <see cref="PFMultiplayerFindLobbies" /> is called with a client-entity.
    /// <para>
    /// This value can be no higher than <c>PFLobbyClientRequestedSearchResultCountUpperLimit</c>.
    /// </para>
    /// <para>
    /// When not specified, the limit on the number of search results is service-defined but will be no greater than
    /// <c>PFLobbyClientRequestedSearchResultCountUpperLimit</c>.
    /// </para>
    /// </remarks>
    _Maybenull_ const uint32_t * clientSearchResultCount;
};

/// <summary>
/// An entry in the collection of lobby search results received upon successful completion of a
/// <see cref="PFMultiplayerFindLobbies" /> operation.
/// </summary>
struct PFLobbySearchResult
{
    /// <summary>
    /// The ID of the found lobby.
    /// </summary>
    _Null_terminated_ const char * lobbyId;

    /// <summary>
    /// The connection string of the found lobby.
    /// </summary>
    /// <remarks>
    /// <c>connectionString</c> can be null. In this case, an invite is required to join.
    /// </remarks>
    _Maybenull_ _Null_terminated_ const char * connectionString;

    /// <summary>
    /// The current owner of the lobby.
    /// </summary>
    /// <remarks>
    /// <c>ownerEntity</c> may be null if the lobby doesn't currently have an owner.
    /// </remarks>
    _Maybenull_ const PFEntityKey * ownerEntity;

    /// <summary>
    /// The maximum number of members that can be present in this lobby.
    /// </summary>
    _Field_range_(PFLobbyMaxMemberCountLowerLimit, PFLobbyMaxMemberCountUpperLimit) uint32_t maxMemberCount;

    /// <summary>
    /// The current number of members that are present in this lobby.
    /// </summary>
    uint32_t currentMemberCount;

    /// <summary>
    /// The number of search properties associated with this lobby.
    /// </summary>
    _Field_range_(0, PFLobbyMaxSearchPropertyCount) uint32_t searchPropertyCount;

    /// <summary>
    /// The keys of the search properties associated with this lobby.
    /// </summary>
    _Field_size_(searchPropertyCount) const char * const * searchPropertyKeys;

    /// <summary>
    /// The values of the search properties associated with this lobby.
    /// </summary>
    _Field_size_(searchPropertyCount) const char * const * searchPropertyValues;

    /// <summary>
    /// The number of friends in the found lobby, if the lobby search was performed with a
    /// <c>PFLobbySearchFriendsFilter</c>.
    /// </summary>
    /// <remarks>
    /// If the lobby search which generated this search result was not performed with a
    /// <c>PFLobbySearchFriendsFilter</c>, this value will always be 0.
    /// <para>
    /// In some multiplayer social networks, friendship is a unidirectional relationship. One user may "follow" another
    /// or be their friend, but the same is not necessarily true in the reverse direction. Friends will only be returned
    /// in this search result when a bidirectional friendship exists. That is, the user querying for the lobby and the
    /// user in the lobby must both be friends with each other.
    /// </para>
    /// </remarks>
    uint32_t friendCount;

    /// <summary>
    /// The list of friends in the found lobby, if the lobby search was performed with a
    /// <c>PFLobbySearchFriendsFilter</c>.
    /// </summary>
    /// <remarks>
    /// If the lobby search which generated this search result was not performed with a
    /// <c>PFLobbySearchFriendsFilter</c>, this list will always be empty.
    /// <para>
    /// In some multiplayer social networks, friendship is a unidirectional relationship. One user may "follow" another
    /// or be their friend, but the same is not necessarily true in the reverse direction. Friends will only be returned
    /// in this search result when a bidirectional friendship exists. That is, the user querying for the lobby and the
    /// user in the lobby must both be friends with each other.
    /// </para>
    /// </remarks>
    _Field_size_(friendCount) const PFEntityKey * friends;
};

/// <summary>
/// A generic, base structure representation of an event or change in state.
/// </summary>
/// <remarks>
/// PFLobbyStateChange structures are reported by <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> for the
/// title to handle and then promptly pass back via the <see cref="PFMultiplayerFinishProcessingLobbyStateChanges()" />
/// method.
/// <para>
/// The <c>stateChangeType</c> field indicates which kind of state change occurred, and this base structure should then
/// be cast to a more specific derived structure to retrieve additional event-specific information.
/// </para>
/// </remarks>
/// <seealso cref="PFMultiplayerStartProcessingLobbyStateChanges" />
/// <seealso cref="PFMultiplayerFinishProcessingLobbyStateChanges" />
struct PFLobbyStateChange
{
    /// <summary>
    /// The specific type of the state change represented.
    /// </summary>
    /// <remarks>
    /// Use this field to determine which corresponding derived structure is represented by this PFLobbyStateChange
    /// structure header.
    /// </remarks>
    PFLobbyStateChangeType stateChangeType;
};

/// <summary>
/// Information specific to the <em>CreateAndJoinLobbyCompleted</em> type of state change.
/// </summary>
struct PFLobbyCreateAndJoinLobbyCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the CreateAndJoinLobby operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;

    /// <summary>
    /// The lobby that was created and joined.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;
};

/// <summary>
/// Information specific to the <em>JoinLobbyCompleted</em> type of state change.
/// </summary>
struct PFLobbyJoinLobbyCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the JoinLobby operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The entity provided to the call associated with this state change which is joining the lobby.
    /// </summary>
    PFEntityKey newMember;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;

    /// <summary>
    /// The lobby that was joined.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;
};

/// <summary>
/// Information specific to the <em>MemberAdded</em> type of state change.
/// </summary>
/// <remarks>
/// This state change will be generated by all operations which add members to lobbies such as
/// <see cref="PFMultiplayerCreateAndJoinLobby" />, <see cref="PFMultiplayerJoinLobby" />, and
/// <see cref="PFLobbyAddMember" />.
/// <para>
/// When this state change is provided by <see cref="PFMultiplayerStartProcessingLobbyStateChanges" /> the lobby will
/// update to reflect the new member in the member list and the member's initial properties will become queryable.
/// </para>
/// </remarks>
struct PFLobbyMemberAddedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The lobby the new member was added to.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The PlayFab entity which is now a member of the lobby.
    /// </summary>
    PFEntityKey member;
};

/// <summary>
/// Information specific to the <em>AddMemberCompleted</em> type of state change.
/// </summary>
struct PFLobbyAddMemberCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the AddMember operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The lobby used in the call associated with this state change.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The local user provided to the call associated with this state change.
    /// </summary>
    PFEntityKey localUser;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;
};

/// <summary>
/// Information specific to the <em>MemberRemoved</em> type of state change.
/// </summary>
/// <remarks>
/// This state change will be generated by all operations which remove members from lobbies such as
/// <see cref="PFLobbyLeave()" /> and <see cref="PFLobbyForceRemoveMember()" />.
/// <para>
/// When this state change is provided by <see cref="PFMultiplayerStartProcessingLobbyStateChanges" /> the lobby will
/// update to remove this member from the member list and the member's properties will be emptied.
/// </para>
/// </remarks>
struct PFLobbyMemberRemovedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The lobby the new member was removed from.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The member entity which has been removed from the lobby.
    /// </summary>
    PFEntityKey member;

    /// <summary>
    /// The reason <c>member</c> was removed from the lobby.
    /// </summary>
    PFLobbyMemberRemovedReason reason;
};

/// <summary>
/// Information specific to the <em>ForceRemoveMemberCompleted</em> type of state change.
/// </summary>
struct PFLobbyForceRemoveMemberCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the ForceRemoveMember operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The lobby provided to the call associated with this state change.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The member entity provided to the call associated with this state change.
    /// </summary>
    PFEntityKey targetMember;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;
};

/// <summary>
/// Information specific to the <em>LeaveLobbyCompleted</em> type of state change.
/// </summary>
struct PFLobbyLeaveLobbyCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The lobby provided to the call associated with this state change.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The local user provided to the call associated with this state change. May be null.
    /// </summary>
    /// <remarks>
    /// If this value is null it signifies that the title requested all local members leave the specified lobby.
    /// </remarks>
    _Maybenull_ const PFEntityKey * localUser;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;
};

/// <summary>
/// Information specific to the <em>Updated</em> type of state change.
/// </summary>
/// <remarks>
/// This state change signifies that the lobby has updated and provides hints as to which values have changed. Multiple
/// updates may be provided by a single call to <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" />. All
/// state reflected by these updates will become available simultaneously when
/// PFMultiplayerStartProcessingLobbyStateChanges() is called, so the updates can be reconciled either individually or
/// as a batch.
/// </remarks>
struct PFLobbyUpdatedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The lobby which was updated.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// A flag indicating if the lobby's owner was updated.
    /// </summary>
    bool ownerUpdated;

    /// <summary>
    /// A flag indicating if the maxmimum number of members allowed in the lobby has been updated.
    /// </summary>
    bool maxMembersUpdated;

    /// <summary>
    /// A flag indicating if the lobby's access policy was updated.
    /// </summary>
    bool accessPolicyUpdated;

    /// <summary>
    /// A flag indicating if the lobby's membership lock has updated.
    /// </summary>
    bool membershipLockUpdated;

    /// <summary>
    /// The number of search properties which have been updated.
    /// </summary>
    _Field_range_(0, PFLobbyMaxSearchPropertyCount * 2) uint32_t updatedSearchPropertyCount;

    /// <summary>
    /// The keys of the search properties which have been updated.
    /// </summary>
    _Field_size_(updatedSearchPropertyCount) const char * const * updatedSearchPropertyKeys;

    /// <summary>
    /// The number of lobby properties which have been updated.
    /// </summary>
    _Field_range_(0, PFLobbyMaxLobbyPropertyCount * 2) uint32_t updatedLobbyPropertyCount;

    /// <summary>
    /// The keys of the lobby properties which have been updated.
    /// </summary>
    _Field_size_(updatedLobbyPropertyCount) const char * const * updatedLobbyPropertyKeys;

    /// <summary>
    /// The number of updates to the lobby members.
    /// </summary>
    uint32_t memberUpdateCount;

    /// <summary>
    /// The set of member updates.
    /// </summary>
    _Field_size_(memberUpdateCount) const PFLobbyMemberUpdateSummary * memberUpdates;
};

/// <summary>
/// Information specific to the <em>PostUpdateCompleted</em> type of state change.
/// </summary>
struct PFLobbyPostUpdateCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the update operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The lobby provided to the call associated with this state change.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The local user provided to the call associated with this state change.
    /// </summary>
    PFEntityKey localUser;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;
};

/// <summary>
/// Information specific to the <em>Disconnecting</em> type of state change.
/// </summary>
/// <remarks>
/// This state change signals that the lobby is in the process of disconnecting because there are no local members
/// actively connected or attempting to reconnect to the lobby. At the point when this state change is provided by
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" />, some operations which require a connected lobby
/// object will begin to fail such as <see cref="PFLobbyAddMember" />.
/// </remarks>
struct PFLobbyDisconnectingStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The lobby that has started disconnecting.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The reason the lobby started disconnecting.
    /// </summary>
    PFLobbyDisconnectingReason reason;
};

/// <summary>
/// Information specific to the <em>Disconnected</em> type of state change.
/// </summary>
/// <remarks>
/// This state change signals that the lobby has completed disconnecting. This is the last state change that will be
/// provided for this lobby object. Once this state change is returned to
/// <see cref="PFMultiplayerFinishProcessingLobbyStateChanges()" />, the Lobby object memory will become invalid.
/// </remarks>
struct PFLobbyDisconnectedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The lobby that has disconnected.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;
};

/// <summary>
/// Information specific to the <em>JoinArrangedLobbyCompleted</em> type of state change.
/// </summary>
struct PFLobbyJoinArrangedLobbyCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the JoinArrangedLobby operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The local member entity provided to the call associated with this state change which is joining this lobby.
    /// </summary>
    PFEntityKey newMember;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;

    /// <summary>
    /// The lobby that was created and joined.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;
};

/// <summary>
/// Information specific to the <em>FindLobbiesCompleted</em> type of state change.
/// </summary>
struct PFLobbyFindLobbiesCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the search lobbies operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The entity provided to the call associated with this state change.
    /// </summary>
    PFEntityKey searchingEntity;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;

    /// <summary>
    /// The number of results returned by the search operation.
    /// </summary>
    /// <para>
    /// For client-entity callers, at most <c>PFLobbyClientRequestedSearchResultCountUpperLimit</c> search results will
    /// ever be returned from a single <see cref="PFMultiplayerFindLobbies" /> operation.
    /// </para>
    uint32_t searchResultCount;

    /// <summary>
    /// The results returned by the search operation.
    /// </summary>
    _Field_size_(searchResultCount) const PFLobbySearchResult * searchResults;
};

/// <summary>
/// Information specific to the <em>InviteReceived</em> type of state change.
/// </summary>
struct PFLobbyInviteReceivedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The entity which is listening for invites and which has been invited.
    /// </summary>
    PFEntityKey listeningEntity;

    /// <summary>
    /// The entity which has invited the <c>listeningEntity</c> to a lobby.
    /// </summary>
    PFEntityKey invitingEntity;

    /// <summary>
    /// The connection string of the lobby to which the <c>listeningEntity</c> has been invited.
    /// </summary>
    _Null_terminated_ const char * connectionString;
};

/// <summary>
/// Information specific to the <em>InviteListenerStatusChanged</em> type of state change.
/// </summary>
struct PFLobbyInviteListenerStatusChangedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// The entity associated with the invite listener.
    /// </summary>
    PFEntityKey listeningEntity;
};

/// <summary>
/// Information specific to the <em>SendInviteCompleted</em> type of state change.
/// </summary>
struct PFLobbySendInviteCompletedStateChange : PFLobbyStateChange
{
    /// <summary>
    /// Indicates the result of the SendInvite operation.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this result can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
    /// </remarks>
    HRESULT result;

    /// <summary>
    /// The lobby that the invite was sent for.
    /// </summary>
    _Notnull_ PFLobbyHandle lobby;

    /// <summary>
    /// The local user which attempted to send the invite.
    /// </summary>
    PFEntityKey sender;

    /// <summary>
    /// The entity which was invited.
    /// </summary>
    PFEntityKey invitee;

    /// <summary>
    /// The async context provided to the call associated with this state change.
    /// </summary>
    void * asyncContext;
};

#pragma pack(pop)

/// <summary>
/// Gets the ID of the Lobby.
/// </summary>
/// <remarks>
/// If this lobby object was created by calling <see cref="PFMultiplayerCreateAndJoinLobby()" /> or
/// <see cref="PFMultiplayerJoinLobby()" />, this method will return an error until
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a successful
/// <see cref="PFLobbyCreateAndJoinLobbyCompletedStateChange" /> or <see cref="PFLobbyJoinLobbyCompletedStateChange" />
/// respectively.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="id">
/// The output lobby ID.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetLobbyId(
    PFLobbyHandle lobby,
    _Outptr_ const char ** id
    ) noexcept;

/// <summary>
/// Gets the max member count of the lobby.
/// </summary>
/// <remarks>
/// If this lobby object was created by calling <see cref="PFMultiplayerJoinLobby()" />, this method will return an
/// error until <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a
/// <see cref="PFLobbyUpdatedStateChange" /> with <see cref="PFLobbyUpdatedStateChange::maxMembersUpdated" /> set to
/// true. If joining the lobby succeeds, this field is guaranteed to be populated by the time
/// PFMultiplayerStartProcessingLobbyStateChanges() provides a <see cref="PFLobbyJoinLobbyCompletedStateChange" />.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="maxMemberCount">
/// The output max member count.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the result can be retrieved
/// via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetMaxMemberCount(
    PFLobbyHandle lobby,
    _Out_ uint32_t * maxMemberCount
    ) noexcept;

/// <summary>
/// Gets the current owner of the lobby.
/// </summary>
/// <remarks>
/// If this lobby object was created by calling <see cref="PFMultiplayerJoinLobby()" />, this method will return an
/// error until <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a
/// <see cref="PFLobbyUpdatedStateChange" /> with <see cref="PFLobbyUpdatedStateChange::ownerUpdated" /> set to true. If
/// joining the lobby succeeds, this field is guaranteed to be populated by the time
/// PFMultiplayerStartProcessingLobbyStateChanges() provides a <see cref="PFLobbyJoinLobbyCompletedStateChange" />.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="owner">
/// The output owner. This value may be null if the owner has left or disconnected from the lobby while the owner
/// migration policy is <see cref="PFLobbyOwnerMigrationPolicy::Manual" /> or
/// <see cref="PFLobbyOwnerMigrationPolicy::None" />.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetOwner(
    PFLobbyHandle lobby,
    _Outptr_result_maybenull_ const PFEntityKey ** owner
    ) noexcept;

/// <summary>
/// Gets the owner migration policy of the lobby.
/// </summary>
/// <remarks>
/// The owner migration policy cannot change for the lifetime of the lobby.
/// <para>
/// If this lobby object was created by calling <see cref="PFMultiplayerJoinLobby()" />, this method will return an
/// error until <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a
/// <see cref="PFLobbyUpdatedStateChange" /> with <see cref="PFLobbyUpdatedStateChange::ownerMigrationPolicy" /> set to
/// true. If joining the lobby succeeds, this field is guaranteed to be populated by the time
/// PFMultiplayerStartProcessingLobbyStateChanges() provides a <see cref="PFLobbyJoinLobbyCompletedStateChange" />.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="ownerMigrationPolicy">
/// The output owner migration policy.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetOwnerMigrationPolicy(
    PFLobbyHandle lobby,
    _Out_ PFLobbyOwnerMigrationPolicy * ownerMigrationPolicy
    ) noexcept;

/// <summary>
/// Gets the access policy of the lobby.
/// </summary>
/// <remarks>
/// If this lobby object was created by calling <see cref="PFMultiplayerJoinLobby()" />, this method will return an
/// error until <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a
/// <see cref="PFLobbyUpdatedStateChange" /> with <see cref="PFLobbyUpdatedStateChange::accessPolicyUpdated" /> set to
/// true. If joining the lobby succeeds, this field is guaranteed to be populated by the time
/// PFMultiplayerStartProcessingLobbyStateChanges() provides a <see cref="PFLobbyJoinLobbyCompletedStateChange" />.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="accessPolicy">
/// The output access policy.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetAccessPolicy(
    PFLobbyHandle lobby,
    _Out_ PFLobbyAccessPolicy * accessPolicy
    ) noexcept;

/// <summary>
/// Gets the lobby's current membership lock state.
/// </summary>
/// <remarks>
/// If this lobby object was created by calling <see cref="PFMultiplayerJoinLobby()" />, this method will return an
/// error until <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a
/// <see cref="PFLobbyUpdatedStateChange" /> with <see cref="PFLobbyUpdatedStateChange::membershipLockUpdated" /> set to
/// true. If joining the lobby succeeds, this field is guaranteed to be populated by the time
/// PFMultiplayerStartProcessingLobbyStateChanges() provides a <see cref="PFLobbyJoinLobbyCompletedStateChange" />.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="lockState">
/// The output membership lock state.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetMembershipLock(
    PFLobbyHandle lobby,
    _Out_ PFLobbyMembershipLock * lockState
    ) noexcept;

/// <summary>
/// Gets the default connection string associated with the lobby.
/// </summary>
/// <remarks>
/// If this lobby object was created by calling <see cref="PFMultiplayerCreateAndJoinLobby()" />, this method will
/// return an error until <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a successful
/// <see cref="PFLobbyCreateAndJoinLobbyCompletedStateChange" />. If this lobby object was created by calling
/// <see cref="PFMultiplayerJoinLobby()" />, this method will return an error until
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> provides a successful
/// <see cref="PFLobbyJoinLobbyCompletedStateChange" />.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="connectionString">
/// The output connection string.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetConnectionString(
    PFLobbyHandle lobby,
    _Outptr_ const char ** connectionString
    ) noexcept;

/// <summary>
/// Gets the list of PlayFab entities currently joined to the lobby as members.
/// </summary>
/// <remarks>
/// If this lobby object is still in the process of asynchronously being created or joined, via a call to either
/// <see cref="PFMultiplayerCreateAndJoinLobby()" /> or <see cref="PFMultiplayerJoinLobby()" /> respectively, this
/// method will return no members.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="memberCount">
/// The output member count.
/// </param>
/// <param name="members">
/// The output member array.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetMembers(
    PFLobbyHandle lobby,
    _Out_ uint32_t * memberCount,
    _Outptr_result_buffer_(*memberCount) const PFEntityKey ** members
    ) noexcept;

/// <summary>
/// Add a local user as a member to the lobby.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbyMemberAddedStateChange" /> followed by a <see cref="PFLobbyAddMemberCompletedStateChange" /> with
/// the <see cref="PFLobbyAddMemberCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed completion,
/// the title will be provided a <see cref="PFLobbyAddMemberCompletedStateChange" /> with the
/// <see cref="PFLobbyAddMemberCompletedStateChange::result" /> field set to a failure hresult.
/// <para>
/// This method is used to add another local PlayFab entity to a pre-existing lobby object. Because the lobby
/// object must have already been created either via a call to <see cref="PFMultiplayerCreateAndJoinLobby" /> or
/// <see cref="PFMultiplayerJoinLobby" />, this method is primarily useful for multiple local user scenarios.
/// </para>
/// <para>
/// This is an asynchronous operation. The member added via this method will not be reflected in the lists returned by
/// <see cref="PFLobbyGetMembers" /> until the asynchronous operation successfully completes and a
/// <c>PFLobbyMemberAddedStateChange</c> struct is provided by
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges" />.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="localUser">
/// The PlayFab Entity Key of the local user to add to the lobby as a member.
/// </param>
/// <param name="memberPropertyCount">
/// The number of initial member properties to set for this user when they join the lobby.
/// </param>
/// <param name="memberPropertyKeys">
/// The keys of the initial member properties to set for this user when they join the lobby.
/// </param>
/// <param name="memberPropertyValues">
/// The values of the initial member properties to set for this user when they join the lobby.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyAddMember(
    PFLobbyHandle lobby,
    const PFEntityKey * localUser,
    _In_range_(0, PFLobbyMaxMemberPropertyCount) uint32_t memberPropertyCount,
    _In_reads_(memberPropertyCount) const char * const * memberPropertyKeys,
    _In_reads_(memberPropertyCount) const char * const * memberPropertyValues,
    _In_opt_ void * asyncContext
    ) noexcept;

/// <summary>
/// Forcibly remove an entity from the lobby.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbyMemberRemovedStateChange" /> followed by a
/// <see cref="PFLobbyForceRemoveMemberCompletedStateChange" /> with the
/// <see cref="PFLobbyForceRemoveMemberCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed
/// completion, the title will be provided a <see cref="PFLobbyForceRemoveMemberCompletedStateChange" /> with the
/// <see cref="PFLobbyForceRemoveMemberCompletedStateChange::result" /> field set to a failure hresult.
/// <para>
/// One of the local PlayFab entities present in this lobby must be the owner for this operation to succeed. If the
/// local owning entity who initiated this operation loses their ownership status while the operation is in progress,
/// the operation will fail asynchronously.
/// </para>
/// <para>
/// This is an asynchronous operation. The member removed via this method will not be removed from the lists returned by
/// <see cref="PFLobbyGetMembers" /> until the asynchronous operation successfully completes and a
/// <c>PFLobbyMemberRemovedStateChange</c> struct is provided by
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges" />.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="targetMember">
/// The member to forcibly remove.
/// </param>
/// <param name="preventRejoin">
/// A flag indicating whether <paramref name="targetMember" /> will be prevented from rejoining the lobby after being
/// removed.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyForceRemoveMember(
    PFLobbyHandle lobby,
    const PFEntityKey * targetMember,
    bool preventRejoin,
    _In_opt_ void * asyncContext
    ) noexcept;

/// <summary>
/// Request one or all local users to leave the lobby.
/// </summary>
/// <remarks>
/// This method queues an asynchronous operation to remove one or all local users from the lobby. On completion, a
/// <see cref="PFLobbyLeaveLobbyCompletedStateChange" /> will be provided indicating that the operation has completed.
/// <para>
/// This method does not guarantee the leave will succeed. The operation may fail due to networking or service errors.
/// If the leave attempt fails but is retriable, the library will continue to retry the leave operation. Otherwise, the
/// local client will disconnect the requested local members from the lobby, but leave them as members. They will remain
/// as disconnected members until they rejoin.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="localUser">
/// An optional value to indicate if a specific local user should leave the lobby. If this value is nullptr, all local
/// users will leave the lobby.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyLeave(
    PFLobbyHandle lobby,
    _In_opt_ const PFEntityKey * localUser,
    _In_opt_ void * asyncContext
    ) noexcept;

/// <summary>
/// Get the list of search property keys.
/// </summary>
/// <remarks>
/// Search properties are visible to non-members of the lobby as metadata which can be used to filter and sort lobby
/// search results.
/// <para>
/// If this lobby object is still in the process of asynchronously being created or joined via a call to
/// <see cref="PFMultiplayerCreateAndJoinLobby()" /> or <see cref="PFMultiplayerJoinLobby()" />, this method will return
/// no keys.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="propertyCount">
/// The output count of search properties.
/// </param>
/// <param name="keys">
/// The output array of search property keys.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetSearchPropertyKeys(
    PFLobbyHandle lobby,
    _Out_ uint32_t * propertyCount,
    _Outptr_result_buffer_(*propertyCount) const char * const ** keys
    ) noexcept;

/// <summary>
/// Get the search property value from its key.
/// </summary>
/// <remarks>
/// Search properties are visible to non-members of the lobby as metadata which can be used to filter and sort lobby
/// search results.
/// <para>
/// If this lobby object is still in the process of asynchronously being created or joined via a call to
/// <see cref="PFMultiplayerCreateAndJoinLobby()" /> or <see cref="PFMultiplayerJoinLobby()" />, this method will return
/// no properties.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="key">
/// The key of the property.
/// </param>
/// <param name="value">
/// The output value associated with the key. An output null value signifies the property wasn't present.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetSearchProperty(
    PFLobbyHandle lobby,
    _Null_terminated_ const char * key,
    _Outptr_result_maybenull_ const char ** value
    ) noexcept;

/// <summary>
/// Get the list of lobby property keys.
/// </summary>
/// <remarks>
/// Lobby properties are only visible to members of the lobby.
/// <para>
/// If this lobby object is still in the process of asynchronously being created or joined via a call to
/// <see cref="PFMultiplayerCreateAndJoinLobby()" /> or <see cref="PFMultiplayerJoinLobby()" />, this method will return
/// no keys.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="propertyCount">
/// The output count of lobby properties.
/// </param>
/// <param name="keys">
/// The output array of lobby property keys.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetLobbyPropertyKeys(
    PFLobbyHandle lobby,
    _Out_ uint32_t * propertyCount,
    _Outptr_result_buffer_(*propertyCount) const char * const ** keys
    ) noexcept;

/// <summary>
/// Get the lobby property value from its key.
/// </summary>
/// <remarks>
/// Lobby properties are only visible to members of the lobby.
/// <para>
/// If this lobby object is still in the process of asynchronously being created or joined via a call to
/// <see cref="PFMultiplayerCreateAndJoinLobby()" /> or <see cref="PFMultiplayerJoinLobby()" />, this method will return
/// no properties.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="key">
/// The key of the property.
/// </param>
/// <param name="value">
/// The output value associated with the key. An output null value signifies the property wasn't present.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetLobbyProperty(
    PFLobbyHandle lobby,
    _Null_terminated_ const char * key,
    _Outptr_result_maybenull_ const char ** value
    ) noexcept;

/// <summary>
/// Get a list of the specified member's property keys.
/// </summary>
/// <remarks>
/// Per-member properties are only visible to members of the lobby.
/// <para>
/// If the member is still in the process of asynchronously joining this lobby either via
/// <see cref="PFMultiplayerCreateAndJoinLobby()" />, <see cref="PFMultiplayerJoinLobby()" />, or
/// <see cref="PFLobbyAddMember" />, this method will return no keys.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="member">
/// The member being queried.
/// </param>
/// <param name="propertyCount">
/// The output count of member properties.
/// </param>
/// <param name="keys">
/// The output array of member property keys.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetMemberPropertyKeys(
    PFLobbyHandle lobby,
    const PFEntityKey * member,
    _Out_ uint32_t * propertyCount,
    _Outptr_result_buffer_(*propertyCount) const char * const ** keys
    ) noexcept;

/// <summary>
/// Get the member property's value from its key.
/// </summary>
/// <remarks>
/// Per-member properties are only visible to members of the lobby.
/// <para>
/// If the member is still in the process of asynchronously joining this lobby either via
/// <see cref="PFMultiplayerCreateAndJoinLobby()" />, <see cref="PFMultiplayerJoinLobby()" />, or
/// <see cref="PFLobbyAddMember()" />, this method will return no properties.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="member">
/// The member whose property bag we want to query.
/// </param>
/// <param name="key">
/// The key of the property.
/// </param>
/// <param name="value">
/// The output value associated with the key. An output null value signifies the property wasn't present.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetMemberProperty(
    PFLobbyHandle lobby,
    const PFEntityKey * member,
    _Null_terminated_ const char * key,
    _Outptr_result_maybenull_ const char ** value
    ) noexcept;

/// <summary>
/// Post an update to the lobby.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbyPostUpdateCompletedStateChange" /> with the
/// <see cref="PFLobbyPostUpdateCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed completion, the
/// title will be provided a <see cref="PFLobbyPostUpdateCompletedStateChange" /> with the
/// <see cref="PFLobbyPostUpdateCompletedStateChange::result" /> field set to a failure. If applying the update would
/// change the state of the lobby, the title will be provided a <see cref="PFLobbyUpdatedStateChange" /> sometime
/// afterwards.
/// <para>
/// This operation completing successfully only indicates that the Lobby service has accepted the update. The title's
/// local view of the Lobby state will not reflect this update until a <see cref="PFLobbyUpdatedStateChange" /> is
/// provided to the title with the updated state.
/// </para>
/// <para>
/// The <paramref name="lobbyUpdate" /> contains fields that can only be modified by the owner of the lobby. This method
/// will return an error if one of those fields is specified and <paramref name="localUser" /> is not the owner of the
/// lobby.
/// </para>
/// <para>
/// If both a <paramref name="lobbyUpdate" /> and <paramref name="memberUpdate" /> are supplied to this method on behalf
/// of a single entity, both updates will happen atomically.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="localUser">
/// The local user posting the update.
/// </param>
/// <param name="lobbyUpdate">
/// An optional update to apply to the shared portion of the lobby on behalf of <paramref name="localUser" />. If this
/// is not provided, <paramref name="memberUpdate" /> must be provided.
/// </param>
/// <param name="memberUpdate">
/// An optional update to apply to the portion of the lobby owned by <paramref name="localUser" />. If this is not
/// provided, <paramref name="lobbyUpdate" /> must be provided.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyPostUpdate(
    PFLobbyHandle lobby,
    const PFEntityKey * localUser,
    _In_opt_ const PFLobbyDataUpdate * lobbyUpdate,
    _In_opt_ const PFLobbyMemberDataUpdate * memberUpdate,
    _In_opt_ void * asyncContext
    ) noexcept;

/// <summary>
/// Send an invite to this lobby from the local user to the invited entity.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbySendInviteCompletedStateChange" /> with the
/// <see cref="PFLobbySendInviteCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed completion, the
/// title will be provided a <see cref="PFLobbySendInviteCompletedStateChange" /> with the
/// <see cref="PFLobbySendInviteCompletedStateChange::result" /> field set to a failure.
/// <para>
/// The <paramref name="sender" /> must be a local user of this lobby which joined from this client.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="sender">
/// The local user sending the invite.
/// </param>
/// <param name="invitee">
/// The invited entity.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbySendInvite(
    PFLobbyHandle lobby,
    const PFEntityKey * sender,
    const PFEntityKey * invitee,
    _In_opt_ void * asyncContext
    ) noexcept;

/// <summary>
/// Retrieves the app's private, custom pointer-sized context value previously associated with this lobby object.
/// </summary>
/// <remarks>
/// If no custom context has been set yet, the value pointed to by <paramref name="customContext" /> is set to nullptr.
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="customContext">
/// The output custom context.
/// </param>
/// <returns>
/// <c>S_OK</c> if retrieving the custom context succeeded or an error code otherwise. The human-readable form of the
/// error code can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
/// <seealso cref="PFLobbySetCustomContext" />
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbyGetCustomContext(
    PFLobbyHandle lobby,
    _Outptr_result_maybenull_ void ** customContext
    ) noexcept;

/// <summary>
/// Configures an optional, custom pointer-sized context value with this lobby object.
/// </summary>
/// <remarks>
/// The custom context is typically used as a "shortcut" that simplifies accessing local, title-specific memory
/// associated with the lobby without requiring a mapping lookup. The value is retrieved using the
/// <see cref="PFLobbyGetCustomContext()" /> method.
/// <para>
/// Any configured value is treated as opaque by the library, and is only valid on the local device; it is not
/// transmitted over the network.
/// </para>
/// </remarks>
/// <param name="lobby">
/// The handle of the lobby.
/// </param>
/// <param name="customContext">
/// An arbitrary, pointer-sized value to store with the player object.
/// </param>
/// <returns>
/// <c>S_OK</c> if configuring the custom context succeeded or an error code otherwise. The human-readable form of the
/// error code can be retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
/// <seealso cref="PFLobbyGetCustomContext" />
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFLobbySetCustomContext(
    PFLobbyHandle lobby,
    _In_opt_ void * customContext
    ) noexcept;

/// <summary>
/// Retrieves an array of all PFLobbyStateChanges to process since the last such call.
/// </summary>
/// <remarks>
/// This method provides the Lobby library an opportunity to synchronize state with remote devices or services, and
/// retrieves a list of all changes currently available for the title since the last call to this method. The title
/// should use the provided array of 0 or more changes to update its own state or UI, and then call
/// <see cref="PFMultiplayerFinishProcessingLobbyStateChanges()" /> with them in a timely manner.
/// <para>
/// Lobby library state exposed by the library can change during this call, so you must be thread-safe in your use of
/// it. For example, invoking PFMultiplayerStartProcessingLobbyStateChanges() on your UI thread at the same time a
/// separate worker thread is looping through the list of lobby members returned by <see cref="PFLobbyGetMembers()" />
/// may result in crashes because PFMultiplayerStartProcessingLobbyStateChanges() can alter the memory associated with
/// the member list. PFMultiplayerStartProcessingLobbyStateChanges() should be called frequently-- at least once per
/// graphics frame. It's designed to execute and return quickly such that it can be called on your main UI thread with
/// negligible impact. For best results, you should also minimize the time you spend handling state changes before
/// calling PFMultiplayerFinishProcessingLobbyStateChanges().
/// </para>
/// <para>
/// Each state change returned by PFMultiplayerStartProcessingLobbyStateChanges() must be returned to
/// PFMultiplayerFinishProcessingLobbyStateChanges() exactly once, but may be returned out of order and may be
/// interleaved with state changes from other calls to PFMultiplayerStartProcessingLobbyStateChanges(). Any resources
/// associated with a specific state change are guaranteed to stay valid until the state change is returned to
/// PFMultiplayerFinishProcessingLobbyStateChanges().
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="stateChangeCount">
/// The output number of PFLobbyStateChange entries for the title to handle in the <paramref name="stateChanges" />
/// array.
/// </param>
/// <param name="stateChanges">
/// A library-allocated output array of all PFLobbyStateChange entries for the title to handle and then pass to
/// <see cref="PFMultiplayerFinishProcessingLobbyStateChanges()" />.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
/// <seealso cref="PFLobbyStateChange" />
/// <seealso cref="PFMultiplayerFinishProcessingLobbyStateChanges" />
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerStartProcessingLobbyStateChanges(
    PFMultiplayerHandle handle,
    _Out_ uint32_t * stateChangeCount,
    _Outptr_result_buffer_(*stateChangeCount) const PFLobbyStateChange * const ** stateChanges
    ) noexcept;

/// <summary>
/// Returns an array of PFLobbyStateChanges that were being processed.
/// </summary>
/// <remarks>
/// This method informs the Lobby library that the state changes reported by a previous call to
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> have now been handled by the title, so their
/// associated resources can be reclaimed. You may call PFMultiplayerFinishProcessingLobbyStateChanges() with any number
/// of state changes. Each state change returned by PFMultiplayerStartProcessingLobbyStateChanges() must be returned to
/// PFMultiplayerFinishProcessingLobbyStateChanges() exactly once, but may be returned out of order and may be
/// interleaved with state changes from other calls to PFMultiplayerStartProcessingLobbyStateChanges().
/// <para>
/// Any resources associated with a specific state change are guaranteed to stay valid until the state change is
/// returned to PFMultiplayerFinishProcessingLobbyStateChanges().
/// </para>
/// <para>
/// For best results, you should minimize the time you spend handling state changes before calling
/// PFMultiplayerFinishProcessingLobbyStateChanges().
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="stateChangeCount">
/// The number of changes, provided in the list specified by <paramref name="stateChanges" />, previously returned by
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> that have now been handled by the title.
/// </param>
/// <param name="stateChanges">
/// The array of changes previously returned by <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" /> that have
/// now been handled by the title.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
/// <seealso cref="PFLobbyStateChange" />
/// <seealso cref="PFMultiplayerStartProcessingLobbyStateChanges" />
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerFinishProcessingLobbyStateChanges(
    PFMultiplayerHandle handle,
    uint32_t stateChangeCount,
    _In_reads_(stateChangeCount) const PFLobbyStateChange * const * stateChanges
    ) noexcept;

/// <summary>
/// Create a new lobby and add the creating PlayFab entity to it.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbyMemberAddedStateChange" /> followed by a
/// <see cref="PFLobbyCreateAndJoinLobbyCompletedStateChange" /> with the
/// <see cref="PFLobbyCreateAndJoinLobbyCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed
/// completion, the title will be provided a <see cref="PFLobbyCreateAndJoinLobbyCompletedStateChange" /> with the
/// <see cref="PFLobbyCreateAndJoinLobbyCompletedStateChange::result" /> field set to a failure.
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="creator">
/// The local PlayFab entity creating the lobby.
/// </param>
/// <param name="createConfiguration">
/// The initial configuration data used when creating the lobby.
/// </param>
/// <param name="joinConfiguration">
/// The initial configuration data for the member creating and joining the lobby.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <param name="lobby">
/// The optional, output lobby object which can be used to queue operations for immediate execution of this operation
/// completes.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerCreateAndJoinLobby(
    PFMultiplayerHandle handle,
    const PFEntityKey * creator,
    const PFLobbyCreateConfiguration * createConfiguration,
    const PFLobbyJoinConfiguration * joinConfiguration,
    _In_opt_ void * asyncContext,
    _Outptr_opt_ PFLobbyHandle * lobby
    ) noexcept;

/// <summary>
/// Join a lobby as the local PlayFab entity.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbyMemberAddedStateChange" /> followed by a <see cref="PFLobbyUpdatedStateChange" /> and
/// <see cref="PFLobbyJoinLobbyCompletedStateChange" /> with the
/// <see cref="PFLobbyJoinLobbyCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed completion, the
/// title will be provided a <see cref="PFLobbyJoinLobbyCompletedStateChange" /> with the
/// <see cref="PFLobbyJoinLobbyCompletedStateChange::result" /> field set to a failure.
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="newMember">
/// The local entity joining the lobby.
/// </param>
/// <param name="connectionString">
/// The connection string used by the entity to join the lobby.
/// </param>
/// <param name="configuration">
/// The initial configuration data used when joining the lobby.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <param name="lobby">
/// The optional, output lobby object which can be used to queue operations for immediate execution of this operation
/// completes.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerJoinLobby(
    PFMultiplayerHandle handle,
    const PFEntityKey * newMember,
    _Null_terminated_ const char * connectionString,
    const PFLobbyJoinConfiguration * configuration,
    _In_opt_ void * asyncContext,
    _Outptr_opt_ PFLobbyHandle * lobby
    ) noexcept;

/// <summary>
/// Joins a lobby using an arrangement string provided by another service, such as matchmaking. If no one has joined the
/// lobby yet, the lobby is initialized using the configuration parameters.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbyMemberAddedStateChange" /> followed by a
/// <see cref="PFLobbyJoinArrangedLobbyCompletedStateChange" /> with the
/// <see cref="PFLobbyJoinArrangedLobbyCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed
/// completion, the title will be provided a <see cref="PFLobbyJoinArrangedLobbyCompletedStateChange" /> with the
/// <see cref="PFLobbyJoinArrangedLobbyCompletedStateChange::result" /> field set to a failure.
/// <para>
/// When using matchmaking through this library, the <see cref="PFMatchmakingMatchDetails::lobbyArrangementString" />
/// can be used with this method to join a lobby with all of the users that have been matched together.
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="newMember">
/// The local PlayFab entity joining the lobby.
/// </param>
/// <param name="arrangementString">
/// The arrangement string used by the entity to join the lobby.
/// </param>
/// <param name="configuration">
/// The initial configuration data used to initialize the lobby, if no one has joined the lobby yet.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <param name="lobby">
/// The optional, output lobby object which can be used to queue operations for immediate execution of this operation
/// completes.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerJoinArrangedLobby(
    PFMultiplayerHandle handle,
    const PFEntityKey * newMember,
    _Null_terminated_ const char * arrangementString,
    const PFLobbyArrangedJoinConfiguration * configuration,
    _In_opt_ void * asyncContext,
    _Outptr_opt_ PFLobbyHandle * lobby
    ) noexcept;

/// <summary>
/// Search for lobbies on behalf of the local user.
/// </summary>
/// <remarks>
/// This is an asynchronous operation. Upon successful completion, the title will be provided a
/// <see cref="PFLobbyFindLobbiesCompletedStateChange" /> with the
/// <see cref="PFLobbyFindLobbiesCompletedStateChange::result" /> field set to <c>S_OK</c>. Upon a failed completion,
/// the title will be provided a <see cref="PFLobbyFindLobbiesCompletedStateChange" /> with the
/// <see cref="PFLobbyFindLobbiesCompletedStateChange::result" /> field set to a failure.
/// <para>
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="searchingEntity">
/// The playfab entity performing the search.
/// </param>
/// <param name="searchConfiguration">
/// The configuration used to filter and sort the searched lobbies.
/// </param>
/// <param name="asyncContext">
/// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change with
/// this call.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerFindLobbies(
    PFMultiplayerHandle handle,
    const PFEntityKey * searchingEntity,
    const PFLobbySearchConfiguration * searchConfiguration,
    _In_opt_ void * asyncContext
    ) noexcept;

/// <summary>
/// Enables the Lobby invite listener for a given entity.
/// </summary>
/// <remarks>
/// This operation will synchronously start listening for invites on behalf of the provided entity. When invites are
/// received, they will be provided via <see cref="PFLobbyInviteReceivedStateChange" /> structs. When the status of the
/// invite listener changes, notifications will be provided via
/// <see cref="PFLobbyInviteListenerStatusChangedStateChange" /> structs.
/// <para>
/// Only invites sent after the listener has been started will be received on this client. Invites sent while this
/// listener is not active will not be queued.
/// </para>
/// <para>
/// Invite listening is, by default, disabled for all entities. This method should be called for each local entity that
/// the title wants to receive Lobby invites.
/// </para>
/// <para>
/// Lobby invites and this invite listener are unrelated to and unaffected by platform invite mechanisms.
/// </para>
/// <para>
/// This method may only be called if the Lobby invite listener is not already enabled for the given entity.
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="listeningEntity">
/// The entity which will listen for invites.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerStartListeningForLobbyInvites(
    PFMultiplayerHandle handle,
    const PFEntityKey * listeningEntity
    ) noexcept;

/// <summary>
/// Disables the Lobby invite listener for a given entity.
/// </summary>
/// <remarks>
/// This operation will synchronously stop listening for invites on behalf of the provided entity.
/// <para>
/// Invite notifications which have already been queued internally will still be provided via the next call to
/// <see cref="PFMultiplayerStartProcessingLobbyStateChanges()" />.
/// </para>
/// <para>
/// Lobby invites and this invite listener are unrelated to and unaffected by platform invite mechanisms.
/// </para>
/// <para>
/// This method may only be called if the Lobby invite listener is already enabled for the given entity.
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="listeningEntity">
/// The entity which is listening for invites.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerStopListeningForLobbyInvites(
    PFMultiplayerHandle handle,
    const PFEntityKey * listeningEntity
    ) noexcept;

/// <summary>
/// Retrieve the status of the entity's invite listener.
/// </summary>
/// <remarks>
/// This value is used to understand the state of an entity's invite listener. If the invite listener encounters a fatal
/// error, non-fatal error, or diagnostic change, the listener's status value will reflect it.
/// <para>
/// When the invite listener's status changes, a <see cref="PFLobbyInviteListenerStatusChangedStateChange" /> struct
/// will be provided by <see cref="PFMultiplayerStartProcessingLobbyStateChanges" />. This method can then be called to
/// retrieve the latest status and act accordingly.
/// </para>
/// </remarks>
/// <param name="handle">
/// The handle of the PFMultiplayer API instance.
/// </param>
/// <param name="listeningEntity">
/// The entity which is listening for invites.
/// </param>
/// <param name="status">
/// The output status value.
/// </param>
/// <returns>
/// <c>S_OK</c> if the call succeeded or an error code otherwise. The human-readable form of the error code can be
/// retrieved via <see cref="PFMultiplayerGetErrorMessage()" />.
/// </returns>
PFMULTIPLAYER_API_ATTRIBUTES
HRESULT
PFMULTIPLAYER_API
PFMultiplayerGetLobbyInviteListenerStatus(
    PFMultiplayerHandle handle,
    const PFEntityKey * listeningEntity,
    _Out_ PFLobbyInviteListenerStatus * status
    ) noexcept;

#ifdef __cplusplus
}
#endif
