#pragma once

#ifndef __cplusplus
#error "This header requires C++"
#endif // end if not defined __cplusplus

#include <PartyTypes.h>

namespace Party {

class PartyXblChatUser;
class PartyXblLocalChatUser;

/// <summary>
/// Convenience type for a constant array of constant PartyXblStateChange pointers.
/// </summary>
/// <seealso cref="PartyXblStateChange" />
typedef const struct PartyXblStateChange * const * PartyXblStateChangeArray;

/// <summary>
/// Convenience type for a constant array of PartyXblChatUser pointers.
/// </summary>
/// <seealso cref="PartyXblChatUser" />
typedef class PartyXblChatUser * const * PartyXblChatUserArray;

/// <summary>
/// Possible cross-network communication privacy settings for a local Xbox Live user.
/// </summary>
/// <remarks>
/// A cross-network user is defined as a user of a non-Xbox Live gaming network.
/// </remarks>
enum class PartyXblCrossNetworkCommunicationPrivacySetting
{
    /// <summary>
    /// Communication with cross-network users is allowed with no restriction for this user.
    /// </summary>
    Allowed,

    /// <summary>
    /// Communication with cross-network users is restricted to friends only for this user.
    /// </summary>
    FriendsOnly,

    /// <summary>
    /// Communication with cross-network users is not allowed for this user.
    /// </summary>
    Disallowed,
};

/// <summary>
/// Reasons communication may be restricted.
/// </summary>
enum class PartyXblChatPermissionMaskReason
{
    /// <summary>
    /// There is no restriction currently applying to this chat permission.
    /// </summary>
    NoRestriction,

    /// <summary>
    /// Communication is restricted while the local chat user's communication privilege and privacy settings are being
    /// queried.
    /// </summary>
    Determining,

    /// <summary>
    /// Communication is restricted due to the local chat user's communication privilege.
    /// </summary>
    Privilege,

    /// <summary>
    /// Communication is restricted due to the local chat user's privacy settings in relation to the target chat user.
    /// </summary>
    Privacy,

    /// <summary>
    /// Communication is restricted because the target user was not recognized as valid by Xbox Live services.
    /// </summary>
    InvalidTargetUser,

    /// <summary>
    /// The required chat permission could not be successfully determined due to issues with Xbox Live services.
    /// </summary>
    XboxLiveServiceError,

    /// <summary>
    /// The required chat permission could not be successfully determined due to an unknown internal error.
    /// </summary>
    UnknownError,
};

/// <summary>
/// Reasons a local chat user object might be destroyed.
/// </summary>
enum class PartyXblLocalChatUserDestroyedReason
{
    /// <summary>
    /// The user is not authorized to communicate with the Xbox Live service.
    /// </summary>
    /// <remarks>
    /// On the Windows 10 platform this can be caused by an invalid token or signature provided to the library by
    /// calling <see cref="PartyXblManager::CompleteGetTokenAndSignatureRequest()" />.
    /// </remarks>
    UnauthorizedLocalChatUser,

    /// <summary>
    /// A valid token and/or signature for an HTTP request could not be acquired.
    /// </summary>
    FailedToGetTokenAndSignatureForLocalChatUser,

    /// <summary>
    /// The Xbox Live service is temporarily unavailable.
    /// </summary>
    XboxLiveServiceTemporarilyUnavailable,

    /// <summary>
    /// The local device has internet connectivity issues which caused the operation to fail.
    /// </summary>
    InternetConnectivityError,

    /// <summary>
    /// The Xbox Live user associated with the PartyXblLocalChatUser is not currently signed-in on the device.
    /// </summary>
    XboxLiveUserNotSignedIn,

    /// <summary>
    /// An unknown error occurred.
    /// </summary>
    UnknownError,
};

/// <summary>
/// Information about an HTTP header.
/// </summary>
struct PartyXblHttpHeader
{
    /// <summary>
    /// The header's name.
    /// </summary>
    PartyString name;

    /// <summary>
    /// The header's value.
    /// </summary>
    PartyString value;
};

/// <summary>
/// Information regarding a required chat permission.
/// </summary>
struct PartyXblChatPermissionInfo
{
    /// <summary>
    /// The chat permission options mask that should be applied to the target chat user.
    /// </summary>
    /// <remarks>
    /// Use a bitwise 'AND' operation against the desired chat permissions to ensure the resulting permissions will
    /// respect Xbox Live policies.
    /// </remarks>
    PartyChatPermissionOptions chatPermissionMask;

    /// <summary>
    /// Extra information regarding the result of a required chat permission query.
    /// </summary>
    PartyXblChatPermissionMaskReason reason;
};

/// <summary>
/// Accessibility settings associated with an Xbox Live account.
/// </summary>
struct PartyXblAccessibilitySettings
{
    /// <summary>
    /// True if the user has enabled speech to text in its Xbox Live settings
    /// </summary>
    PartyBool speechToTextEnabled;

    /// <summary>
    /// True if the user has enabled text to speech in its Xbox Live settings
    /// </summary>
    PartyBool textToSpeechEnabled;

    /// <summary>
    /// Language code associated with this user's accessibility settings. Can be used as a hint when choosing the
    /// PartyTextToSpeechProfile for this user.
    /// </summary>
    char languageCode[c_maxLanguageCodeStringLength + 1];

    /// <summary>
    /// Gender associated with this user's accessibility settings. Can be used as a hint when choosing the
    /// PartyTextToSpeechProfile for this user.
    /// </summary>
    PartyGender gender;
};

/// <summary>
/// Results for operations that generate state changes.
/// </summary>
enum class PartyXblStateChangeResult
{
    /// <summary>
    /// The operation succeeded.
    /// </summary>
    Succeeded,

    /// <summary>
    /// An unknown error occurred.
    /// </summary>
    UnknownError,

    /// <summary>
    /// Another title operation canceled this operation.
    /// </summary>
    CanceledByTitle,

    /// <summary>
    /// The Xbox Live user or PlayFab Entity associated with the PartyXblLocalChatUser is not authorized to perform this
    /// request.
    /// </summary>
    UserNotAuthorized,

    /// <summary>
    /// The operation failed because too many logins have been attempted for this title.
    /// </summary>
    LoginToPlayFabThrottled,

    /// <summary>
    /// The operation failed because of an unexpected error in the Party Service.
    /// </summary>
    PartyServiceError,

    /// <summary>
    /// The Xbox Live service is temporarily unavailable.
    /// </summary>
    XboxLiveServiceTemporarilyUnavailable,

    /// <summary>
    /// The local device has internet connectivity issues which caused the operation to fail.
    /// </summary>
    InternetConnectivityError,

    /// <summary>
    /// The rate limit for the PlayFab Service has been reached.
    /// </summary>
    PlayFabRateLimitExceeded,
};

/// <summary>
/// Mapping between an Xbox Live User Id and a PlayFab Entity Id.
/// </summary>
struct PartyXblXboxUserIdToPlayFabEntityIdMapping
{
    /// <summary>
    /// The Xbox Live User Id.
    /// </summary>
    uint64_t xboxLiveUserId;

    /// <summary>
    /// The PlayFab Entity Id.
    /// </summary>
    /// <remarks>
    /// This value may be an empty string in the case where the Xbox Live user does not have a linked PlayFab account. A
    /// PlayFab account is automatically created and linked when calling <see cref="PartyXblManager::LoginToPlayFab" />
    /// for the first time. The <b>LoginWithXbox</b> API provided by the PlayFab SDK can also be used to link an
    /// account.
    /// </remarks>
    PartyString playfabEntityId;
};

/// <summary>
/// Threads that Party Xbox Live Helper library uses for internal purposes.
/// </summary>
/// <seealso cref="PartyXblManager::GetThreadAffinityMask" />
/// <seealso cref="PartyXblManager::SetThreadAffinityMask" />
enum class PartyXblThreadId
{
    /// <summary>
    /// Represents the Party Xbox Live Helper library internal web request threads.
    /// </summary>
    /// <remarks>
    /// The internal networking thread is driven from both network I/O and polling mechanisms. It wakes every 50 to 100
    /// milliseconds or whenever network traffic is received.
    /// </remarks>
    WebRequest,
};

/// <summary>
/// The types of state changes that can occur.
/// </summary>
enum class PartyXblStateChangeType : uint32_t
{
    /// <summary>
    /// The operation started by a previous call to <see cref="PartyXblManager::CreateLocalChatUser()"/> completed.
    /// </summary>
    /// <remarks>
    /// The PartyXblStateChange object should be cast to a
    /// <see cref="PartyXblCreateLocalChatUserCompletedStateChange"/>.
    /// </remarks>
    CreateLocalChatUserCompleted,

    /// <summary>
    /// A local chat user was destroyed.
    /// </summary>
    /// <remarks>
    /// The PartyXblStateChange object should be cast to a <see cref="PartyXblLocalChatUserDestroyedStateChange" />
    /// object for more information.
    /// </remarks>
    LocalChatUserDestroyed,

    /// <summary>
    /// Chat permissions between a local chat user and a target chat user has changed.
    /// </summary>
    /// <remarks>
    /// The PartyXblStateChange object should be cast to a
    /// <see cref="PartyXblRequiredChatPermissionInfoChangedStateChange" /> object for more information.
    /// </remarks>
    RequiredChatPermissionInfoChanged,

    /// <summary>
    /// The Xbox Live Helper library requires an Xbox Live token and signature to complete an HTTP request.
    /// </summary>
    /// <remarks>
    /// The title can use the Xbox Authentication Library (XAL) to fulfill these request by calling the
    /// <c>XalUserGetTokenAndSignatureSilentlyAsync</c> function.
    /// <para>
    /// The title needs to call <see cref="PartyXblManager::CompleteGetTokenAndSignatureRequest()" /> with the request
    /// token and signature.
    /// </para>
    /// <para>
    /// The PartyXblStateChange object should be cast to a <see cref="PartyXblTokenAndSignatureRequestedStateChange" />
    /// object for more information.
    /// </para>
    /// <para>
    /// This state change is only returned on the Windows 10 platform.
    /// </para>
    /// </remarks>
    TokenAndSignatureRequested,

    /// <summary>
    /// The operation started by a previous call to <see cref="PartyXblManager::LoginToPlayFab()" /> completed.
    /// </summary>
    /// <remarks>
    /// The PartyXblStateChange object should be cast to a <see cref="PartyXblLoginToPlayFabCompletedStateChange" />
    /// object for more information.
    /// </remarks>
    LoginToPlayFabCompleted,

    /// <summary>
    /// The operation started by a previous call to <see cref="PartyXblManager::GetEntityIdsFromXboxLiveUserIds()" />
    /// completed.
    /// </summary>
    /// <remarks>
    /// The PartyXblStateChange object should be cast to a
    /// <see cref="PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange" /> object for more information.
    /// </remarks>
    GetEntityIdsFromXboxLiveUserIdsCompleted,
};

/// <summary>
/// A generic, base structure representation of an event or change in state.
/// </summary>
/// <remarks>
/// PartyXblStateChange structures are reported by <see cref="PartyXblManager::StartProcessingStateChanges()" /> for the
/// title to handle and then promptly pass back via the <see cref="PartyXblManager::FinishProcessingStateChanges()" />
/// method.
/// <para>
/// The <c>stateChangeType</c> field indicates which kind of state change occurred, and this base structure should then
/// be cast to a more specific derived structure to retrieve additional event-specific information.
/// </para>
/// </remarks>
/// <seealso cref="PartyXblManager::StartProcessingStateChanges" />
/// <seealso cref="PartyXblManager::FinishProcessingStateChanges" />
struct PartyXblStateChange
{
    /// <summary>
    /// The specific type of the state change represented.
    /// </summary>
    /// <remarks>
    /// Use this field to determine which corresponding derived structure is represented by this PartyXblStateChange
    /// structure header.
    /// </remarks>
    PartyXblStateChangeType stateChangeType;
};

/// <summary>
/// Information specific to the <em>CreateLocalChatUserCompleted</em> type of state change.
/// </summary>
struct PartyXblCreateLocalChatUserCompletedStateChange : PartyXblStateChange
{
    /// <summary>
    /// Indicates that the local chat user creation operation Succeeded or provides the reason that it failed.
    /// </summary>
    PartyXblStateChangeResult result;

    /// <summary>
    /// A diagnostic value providing additional troubleshooting information regarding any potential error condition.
    /// </summary>
    PartyError errorDetail;

    /// <summary>
    /// The async identifier provided to the call associated with this state change.
    /// </summary>
    void * asyncIdentifier;

    /// <summary>
    /// The local chat user that was created.
    /// </summary>
    PartyXblLocalChatUser * localChatUser;
};

/// <summary>
/// Information specific to the <em>LoginToPlayFabCompleted</em> type of state change.
/// </summary>
struct PartyXblLoginToPlayFabCompletedStateChange : PartyXblStateChange
{
    /// <summary>
    /// Indicates that the login operation succeeded or the reason that it failed.
    /// </summary>
    PartyXblStateChangeResult result;

    /// <summary>
    /// A diagnostic value providing additional troubleshooting information regarding any potential error condition.
    /// </summary>
    PartyError errorDetail;

    /// <summary>
    /// The local chat user who was logged in to PlayFab.
    /// </summary>
    PartyXblLocalChatUser * localChatUser;

    /// <summary>
    /// The async identifier provided to the call associated with this state change.
    /// </summary>
    void * asyncIdentifier;

    /// <summary>
    /// The PlayFab Entity Id associated with the local chat user's Xbox Live identity.
    /// </summary>
    PartyString entityId;

    /// <summary>
    /// The PlayFab Entity Token associated with the local chat user's Xbox Live identity.
    /// </summary>
    PartyString titlePlayerEntityToken;

    /// <summary>
    /// The timestamp describing when this token expires.
    /// </summary>
    /// <remarks>
    /// When the token is nearing the expiration time or if the token has expired due to the application being in a
    /// dormant state for an extended time, a new token should be obtained by calling
    /// <see cref="PartyXblManager::LoginToPlayFab()" />.
    /// </remarks>
    time_t expirationTime;
};

/// <summary>
/// Information specific to the <em>GetEntityIdsFromXboxLiveUserIdsCompleted</em> type of state change.
/// </summary>
struct PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange : PartyXblStateChange
{
    /// <summary>
    /// Indicates that the operation succeeded or the reason that it failed.
    /// </summary>
    PartyXblStateChangeResult result;

    /// <summary>
    /// A diagnostic value providing additional troubleshooting information regarding any potential error condition.
    /// </summary>
    PartyError errorDetail;

    /// <summary>
    /// The device's Xbox Live sandbox which was used for this operation.
    /// </summary>
    PartyString xboxLiveSandbox;

    /// <summary>
    /// The local chat user that was used to communicate with PlayFab.
    /// </summary>
    PartyXblLocalChatUser * localChatUser;

    /// <summary>
    /// The async identifier provided to the call associated with this state change.
    /// </summary>
    void * asyncIdentifier;

    /// <summary>
    /// The number of Xbox Live User Id to PlayFab Entity Id mappings.
    /// </summary>
    uint32_t entityIdMappingCount;

    /// <summary>
    /// A list of Xbox Live User Id to PlayFab Entity Id mappings.
    /// </summary>
    _Field_size_(entityIdMappingCount) const PartyXblXboxUserIdToPlayFabEntityIdMapping * entityIdMappings;
};

/// <summary>
/// Information specific to the <em>LocalChatUserDestroyed</em> type of state change.
/// </summary>
struct PartyXblLocalChatUserDestroyedStateChange : PartyXblStateChange
{
    /// <summary>
    /// The local chat user provided to the call associated with this state change.
    /// </summary>
    PartyXblLocalChatUser * localChatUser;

    /// <summary>
    /// The reason the local chat user was destroyed.
    /// </summary>
    PartyXblLocalChatUserDestroyedReason reason;

    /// <summary>
    /// A diagnostic value providing additional troubleshooting information regarding any potential error condition.
    /// </summary>
    /// <remarks>
    /// The human-readable form of this error detail can be retrieved via
    /// <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </remarks>
    PartyError errorDetail;
};

/// <summary>
/// Information specific to the <em>RequiredChatPermissionInfoChanged</em> type of state change.
/// </summary>
/// <remarks>
/// Use <see cref="PartyXblLocalChatUser::GetRequiredChatPermissionInfo()" /> with the <c>targetChatUser</c> provided in
/// this state change to get the new chat permission information.
/// </remarks>
struct PartyXblRequiredChatPermissionInfoChangedStateChange : PartyXblStateChange
{
    /// <summary>
    /// The local chat user for which the chat permission info applies to.
    /// </summary>
    PartyXblLocalChatUser * localChatUser;

    /// <summary>
    /// The chat user the chat permission info should be applied to.
    /// </summary>
    PartyXblChatUser * targetChatUser;
};

/// <summary>
/// Information specific to the <em>TokenAndSignatureRequested</em> type of state change.
/// </summary>
/// <remarks>
/// The title can use the Xbox Authentication Library (XAL) to fulfil these requests by calling the
/// XalUserGetTokenAndSignatureSilentlyAsync() function.
/// <para>
/// Use <see cref="PartyXblManager::CompleteGetTokenAndSignatureRequest()" /> to provide the token and signature and
/// complete the operation.
/// </para>
/// </remarks>
struct PartyXblTokenAndSignatureRequestedStateChange : PartyXblStateChange
{
    /// <summary>
    /// Opaque identifier used to associate the token and signature request with the internal HTTP request. Use this
    /// identifier when calling <see cref="PartyXblManager::CompleteGetTokenAndSignatureRequest()" /> to complete the
    /// operation.
    /// </summary>
    uint32_t correlationId;

    /// <summary>
    /// The HTTP method for the request, such as "GET" or "POST".
    /// </summary>
    PartyString method;

    /// <summary>
    /// The URL for the HTTP request that needs a token and signature (fully escaped).
    /// </summary>
    PartyString url;

    /// <summary>
    /// The number of headers for the HTTP request.
    /// </summary>
    uint32_t headerCount;

    /// <summary>
    /// The headers for the HTTP request.
    /// </summary>
    _Field_size_(headerCount) PartyXblHttpHeader * headers;

    /// <summary>
    /// The size of the request body in bytes.
    /// </summary>
    uint32_t bodySize;

    /// <summary>
    /// The request body.
    /// </summary>
    _Field_size_bytes_(bodySize) const void * body;

    /// <summary>
    /// If <c>true</c> the token must be refreshed, ignoring any cached token.
    /// </summary>
    PartyBool forceRefresh;

    /// <summary>
    /// If <c>true</c>, a token should be requested for all users rather than a specific user. In the case
    /// <c>localChatUser</c> will be nullptr.
    /// </summary>
    PartyBool allUsers;

    /// <summary>
    /// The local chat user that should be used to retrieve the token and signature.
    /// </summary>
    /// <remarks>
    /// This field is only valid when <c>allUsers</c> is set to <c>false</c>.
    /// </remarks>
    _Maybenull_ PartyXblLocalChatUser * localChatUser;
};

/// <summary>
/// The management class for Xbox Live operations related to a chat user.
/// </summary>
/// <seealso cref="PartyXblLocalChatUser" />
class PartyXblChatUser
{
public:
    /// <summary>
    /// Gets the PartyXblLocalChatUser version of this chat user.
    /// </summary>
    /// <param name="localChatUser">
    /// The output local version of this chat user object, or nullptr if this is not a local chat user.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </returns>
    PartyError GetLocal(
        _Outptr_result_maybenull_ PartyXblLocalChatUser ** localChatUser
        ) const party_no_throw;

    /// <summary>
    /// Gets the Xbox Live User Id associated with this user.
    /// </summary>
    /// <param name="xboxUserId">
    /// The output Xbox Live User Id.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </returns>
    PartyError GetXboxUserId(
        _Out_ uint64_t * xboxUserId
        ) party_no_throw;

    /// <summary>
    /// Retrieves the app's private, custom pointer-sized context value previously associated with this local chat user
    /// object.
    /// </summary>
    /// <remarks>
    /// If no custom context has been set yet, the value pointed to by <paramref name="customContext" /> is set to
    /// nullptr.
    /// </remarks>
    /// <param name="customContext">
    /// The output custom context.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if retrieving the custom context succeeded or an error code otherwise. The
    /// human-readable form of the error code can be retrieved via <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblChatUser::SetCustomContext" />
    PartyError GetCustomContext(
        _Outptr_result_maybenull_ void ** customContext
        ) const party_no_throw;

    /// <summary>
    /// Configures an optional, custom pointer-sized context value with this local chat user object.
    /// </summary>
    /// <remarks>
    /// The custom context is typically used as a "shortcut" that simplifies accessing local, title-specific memory
    /// associated with the chat user without requiring a mapping lookup. The value is retrieved using the
    /// <see cref="GetCustomContext()" /> method.
    /// <para>
    /// Any configured value is treated as opaque by the library, and is only valid on the local device; it is not
    /// transmitted over the network.
    /// </para>
    /// </remarks>
    /// <param name="customContext">
    /// An arbitrary, pointer-sized value to store with the chat user object.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if configuring the custom context succeeded or an error code otherwise. The
    /// human-readable form of the error code can be retrieved via <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblChatUser::GetCustomContext" />
    PartyError SetCustomContext(
        _In_opt_ void * customContext
        ) party_no_throw;

private:
    PartyXblChatUser() = delete;
    PartyXblChatUser(const PartyXblChatUser&) = delete;
    PartyXblChatUser(PartyXblChatUser&&) = delete;
    PartyXblChatUser& operator=(const PartyXblChatUser&) = delete;
    PartyXblChatUser& operator=(PartyXblChatUser&&) = delete;
    ~PartyXblChatUser() = delete;
};

/// <summary>
/// The management class for Xbox Live operations related to a local chat user. Inherits from
/// <see cref="PartyXblChatUser" />.
/// </summary>
/// <seealso cref="PartyXblChatUser" />
/// <seealso cref="PartyXblChatUser::GetLocal" />
class PartyXblLocalChatUser : public PartyXblChatUser
{
public:
    /// <summary>
    /// Returns the accessibility settings associated with this chat user.
    /// </summary>
    /// <remarks>
    /// This function will fail until a <see cref="PartyXblCreateLocalChatUserCompletedStateChange" /> is provided by
    /// <see cref="PartyXblManager::StartProcessingStateChanges()" />.
    /// </remarks>
    /// <param name="settings">
    /// The output accessibility settings.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblCreateLocalChatUserCompletedStateChange" />
    PartyError GetAccessibilitySettings(
        _Out_ PartyXblAccessibilitySettings * settings
        ) party_no_throw;

    /// <summary>
    /// Gets the cached required chat permission information related to a target chat user.
    /// </summary>
    /// <param name="targetChatUser">
    /// The chat user for which permissions relative to the local chat user should be retrieved.
    /// </param>
    /// <param name="chatPermissionInfo">
    /// The output chat permission info object. The chat permission info object contains the most permissive chat
    /// permission possible while still respecting Xbox Live requirements.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblCreateLocalChatUserCompletedStateChange" />
    /// <seealso cref="PartyXblRequiredChatPermissionInfoChangedStateChange" />
    PartyError GetRequiredChatPermissionInfo(
        const PartyXblChatUser * targetChatUser,
        _Out_ PartyXblChatPermissionInfo * chatPermissionInfo
        ) party_no_throw;

    /// <summary>
    /// Gets the cross-network communication privacy setting for this chat user.
    /// </summary>
    /// <remarks>
    /// A cross-network user is defined as a user of a non-Xbox Live gaming network.
    /// </remarks>
    /// <param name="setting">
    /// The output cross-network communication privacy setting.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="PartyXblManager::GetErrorMessage()" />.
    /// </returns>
    PartyError GetCrossNetworkCommunicationPrivacySetting(
        _Out_ PartyXblCrossNetworkCommunicationPrivacySetting * setting
        ) party_no_throw;

private:
    PartyXblLocalChatUser() = delete;
    PartyXblLocalChatUser(const PartyXblLocalChatUser&) = delete;
    PartyXblLocalChatUser(PartyXblLocalChatUser&&) = delete;
    PartyXblLocalChatUser& operator=(const PartyXblLocalChatUser&) = delete;
    PartyXblLocalChatUser& operator=(PartyXblLocalChatUser&&) = delete;
    ~PartyXblLocalChatUser() = delete;
};

/// <summary>
/// The primary management class for interacting with the Party Xbox Live Helper library.
/// </summary>
/// <remarks>
/// Only a single instance of the class is permitted.
/// </remarks>
class PartyXblManager
{
public:
    /// <summary>
    /// Retrieves a reference to the PartyXblManager singleton instance.
    /// </summary>
    /// <returns>
    /// The PartyXblManager singleton instance.
    /// </returns>
    static PartyXblManager& GetSingleton(
        ) party_no_throw;

    /// <summary>
    /// Get the human-readable form of an error.
    /// </summary>
    /// <remarks>
    /// These error messages are not localized and are only intended for developers, i.e. these error messages are not
    /// intended to be shown to users via UI.
    /// </remarks>
    /// <param name="error">
    /// An error code.
    /// </param>
    /// <param name="errorMessage">
    /// The output, human-readable error message. The memory for the returned string remains valid for the lifetime of
    /// the process.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise.
    /// </returns>
    static PartyError GetErrorMessage(
        PartyError error,
        _Outptr_ PartyString * errorMessage
        ) party_no_throw;

    /// <summary>
    /// Optionally configures the memory allocation and freeing callbacks the Party Xbox Live Helper library should use.
    /// </summary>
    /// <remarks>
    /// This method allows the application to install custom memory allocation routines in order to service all requests
    /// by the Party Xbox Live Helper library for new memory buffers instead of using its default allocation routines.
    /// <para>
    /// The <paramref name="allocateMemoryCallback" /> and <paramref name="freeMemoryCallback" /> parameters must both
    /// be non-null.
    /// </para>
    /// <para>
    /// To use this method, it must be called before any other Party Xbox Live Helper library method except for
    /// <see cref="GetMemoryCallbacks()" />. This method cannot be called again for the lifetime of this process.
    /// </para>
    /// </remarks>
    /// <param name="allocateMemoryCallback">
    /// A pointer to the custom allocation callback to use.
    /// </param>
    /// <param name="freeMemoryCallback">
    /// A pointer to the custom freeing callback to use.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise.
    /// </returns>
    /// <seealso cref="PartyXblManager::GetMemoryCallbacks" />
    static PartyError SetMemoryCallbacks(
        _In_ PartyAllocateMemoryCallback allocateMemoryCallback,
        _In_ PartyFreeMemoryCallback freeMemoryCallback
        ) party_no_throw;

    /// <summary>
    /// Retrieves the current memory allocation and freeing callbacks the Party Xbox Live library is using.
    /// </summary>
    /// <remarks>
    /// This method does not require the <see cref="Initialize()" /> method to have been called first.
    /// </remarks>
    /// <param name="allocateMemoryCallback">
    /// A place to store a pointer to the memory allocation callback currently used.
    /// </param>
    /// <param name="freeMemoryCallback">
    /// A place to store a pointer to the memory freeing callback currently used.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblManager::SetMemoryCallbacks" />
    static PartyError GetMemoryCallbacks(
        _Out_ PartyAllocateMemoryCallback * allocateMemoryCallback,
        _Out_ PartyFreeMemoryCallback * freeMemoryCallback
        ) party_no_throw;

    /// <summary>
    /// Optionally configures the processor on which internal Party Xbox Live Helper library threads will run.
    /// </summary>
    /// <remarks>
    /// This method enables the application to configure the processor affinity for internal Party Xbox Live Helper
    /// library thread of a given type.
    /// <para>
    /// This method may be called at any time before or after <see cref="Initialize()" /> and will take effect
    /// immediately. Thread processor settings are persisted across calls to <see cref="Cleanup()" /> and Initialize().
    /// When there are more than 64 cores present, this method always applies to processor group 0.
    /// </para>
    /// <para>
    /// In order to specify any processor, pass <c>c_anyProcessor</c> as the <paramref name="threadAffinityMask" />
    /// parameter. This is also the default value the Party Xbox Live Helper library will use if this method is never
    /// called.
    /// </para>
    /// </remarks>
    /// <param name="threadId">
    /// The type of internal library thread for which processor affinity should be retrieved.
    /// </param>
    /// <param name="threadAffinityMask">
    /// The affinity mask for this type of Party thread.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblThreadId" />
    /// <seealso cref="PartyXblManager::GetThreadAffinityMask" />
    static PartyError SetThreadAffinityMask(
        PartyXblThreadId threadId,
        uint64_t threadAffinityMask
        ) party_no_throw;

    /// <summary>
    /// Retrieves the current set of processors on which internal Party Xbox Live Helper library threads will run or are
    /// running as an affinity mask.
    /// </summary>
    /// <remarks>
    /// This retrieves the current processor affinity for internal Party threads Xbox Live Helper library of a given
    /// type.
    /// <para>
    /// This method does not require <see cref="Initialize()" /> to have been called first.
    /// </para>
    /// <para>
    /// A reported value of <c>c_anyProcessor</c> written to <paramref name="threadAffinityMask" /> indicates that the
    /// thread is free to run on any processor.
    /// </para>
    /// </remarks>
    /// <param name="threadId">
    /// The type of internal library thread for which processor affinity should be retrieved.
    /// </param>
    /// <param name="threadAffinityMask">
    /// A place to store the affinity mask for this type of Party thread.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblManager::SetThreadAffinityMask" />
    static PartyError GetThreadAffinityMask(
        PartyXblThreadId threadId,
        _Out_ uint64_t * threadAffinityMask
        ) party_no_throw;

    /// <summary>
    /// Initializes the object instance.
    /// </summary>
    /// <remarks>
    /// This must be called before any other method under the PartyXblManager namespace, aside from the static methods
    /// <see cref="GetSingleton()" />, <see cref="SetMemoryCallbacks()" />, <see cref="GetMemoryCallbacks()" />,
    /// <see cref="SetThreadAffinityMask()" />, <see cref="GetThreadAffinityMask()" />. Initialize() cannot be called
    /// again without a subsequent <see cref="Cleanup()" /> call.
    /// <para>
    /// Every call to Initialize() should have a corresponding Cleanup() call.
    /// </para>
    /// <para>
    /// It is recommended for apps using the Xbox One XDK version of the Party Xbox Live Helper library to wait until
    /// the platform is ready for networking operations before calling this method. Please refer to the XDK
    /// documentation about networking and secure device associations best practices for more information.
    /// </para>
    /// <para>
    /// Apps using the Microsoft Game Core version of the Party Xbox Live Helper library will need to wait for the Game
    /// Core Networking stack to be initialized prior to calling this method. Determining the status of the network
    /// stack can be done using the Game Core XNetworkingGetConnectivityHint API.
    /// </para>
    /// <para>
    /// Apps using the Microsoft Game Core version of the Party Xbox Live Helper library must listen for app state
    /// notifications via the RegisterAppStateChangeNotification API. When the app is suspended, the title must call
    /// Cleanup(). When the app is resumed, the title must wait for the Game Core networking stack to be ready and then
    /// re-initialize the Party Xbox Live Helper library by calling Initialize().
    /// </para>
    /// </remarks>
    /// <param name="titleId">
    /// The app's PlayFab Title Id.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblManager::Cleanup" />
    /// <seealso cref="PartyXblManager::GetSingleton" />
    /// <seealso cref="PartyXblManager::SetMemoryCallbacks" />
    /// <seealso cref="PartyXblManager::GetMemoryCallbacks" />
    /// <seealso cref="PartyXblManager::SetThreadAffinityMask" />
    /// <seealso cref="PartyXblManager::GetThreadAffinityMask" />
    PartyError Initialize(
        PartyString titleId
        ) party_no_throw;

    /// <summary>
    /// Immediately reclaims all resources associated with the object.
    /// </summary>
    /// <remarks>
    /// Every call to <see cref="Initialize()" /> should have a corresponding Cleanup() call.
    /// <para>
    /// Titles using the Microsoft Game Core version of the Party Xbox Live Helper library must listen for app state
    /// notifications via the RegisterAppStateChangeNotification API. When the app is suspended, the title must call
    /// Cleanup(). When the app is resumed, the title must wait for the Game Core networking stack to be ready and then
    /// re-initialize the Party Xbox Live Helper library by calling Initialize().
    /// </para>
    /// </remarks>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblManager::Initialize" />
    PartyError Cleanup(
        ) party_no_throw;

    /// <summary>
    /// Retrieves an array of all Party Xbox Live state changes to process since the last such call.
    /// </summary>
    /// <remarks>
    /// This method provides the Party Xbox Live Helper library an opportunity to retrieves a list of all changes
    /// currently available for the title since the last call to this method. The title should use the provided array of
    /// 0 or more changes to update its own state or UI, and then call <see cref="FinishProcessingStateChanges()" />
    /// with them in a timely manner.
    /// <para>
    /// State exposed by the library can change during this call, so you must be thread-safe in your use of it.
    /// StartProcessingStateChanges() should be called frequently-- at least once every 100 milliseconds. It's designed
    /// to execute and return quickly such that it can be called on your main UI thread with negligible impact. For best
    /// results, you should also minimize the time you spend handling state changes before calling
    /// FinishProcessingStateChanges().
    /// </para>
    /// <para>
    /// Each state change returned by StartProcessingStateChanges() must be returned to FinishProcessingStateChanges()
    /// exactly once, but may be returned out of order and may be interleaved with state changes from other calls to
    /// StartProcessingStateChanges(). Any resources associated with a specific state change are guaranteed to stay
    /// valid until the state change is returned to FinishProcessingStateChanges().
    /// </para>
    /// </remarks>
    /// <param name="stateChangeCount">
    /// A place to write the number of PartyXblStateChange entries for the title to handle in the
    /// <paramref name="stateChanges" /> array.
    /// </param>
    /// <param name="stateChanges">
    /// A place to store a pointer to the array of all PartyXblStateChange entries for the title to handle and then pass
    /// to FinishProcessingStateChanges().
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblStateChange" />
    /// <seealso cref="PartyXblManager::FinishProcessingStateChanges" />
    PartyError StartProcessingStateChanges(
        _Out_ uint32_t * stateChangeCount,
        _Outptr_result_buffer_(*stateChangeCount) PartyXblStateChangeArray * stateChanges
        ) party_no_throw;

    /// <summary>
    /// Returns an array of Party Xbox Live state changes that were being processed.
    /// </summary>
    /// <remarks>
    /// This method informs the Party Xbox Live Helper library that the state changes reported by a previous call to
    /// <see cref="StartProcessingStateChanges()" /> have now been handled by the title, so their associated resources
    /// can be reclaimed. You may call FinishProcessingStateChanges() with any number of state changes. Each state
    /// change returned by StartProcessingStateChanges() must be returned to FinishProcessingStateChanges() exactly
    /// once, but may be returned out of order and may be interleaved with state changes from other calls to
    /// StartProcessingStateChanges(). Even if state changes are held across subsequent calls to
    /// StartProcessingStateChanges(), the Party Xbox Live Helper library state returned by all getters will advance and
    /// may no longer reflect the same state that the held state changes refer to.
    /// <para>
    /// Any resources associated with a specific state change are guaranteed to stay valid until the state change is
    /// returned to FinishProcessingStateChanges().
    /// </para>
    /// <para>
    /// For best results, you should minimize the time you spend handling state changes before calling
    /// FinishProcessingStateChanges().
    /// </para>
    /// </remarks>
    /// <param name="stateChangeCount">
    /// The number of changes, provided in the list specified by <paramref name="stateChanges" />, previously returned
    /// by StartProcessingStateChanges() that have now been handled by the title.
    /// </param>
    /// <param name="stateChanges">
    /// The pointer to the array of changes previously returned by StartProcessingStateChanges() that have now been
    /// handled by the title.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblStateChange" />
    /// <seealso cref="PartyXblManager::StartProcessingStateChanges" />
    PartyError FinishProcessingStateChanges(
        uint32_t stateChangeCount,
        _In_reads_(stateChangeCount) PartyXblStateChangeArray stateChanges
        ) party_no_throw;

    /// <summary>
    /// Completes a GetTokenAndSignature operation.
    /// </summary>
    /// <remarks>
    /// The title can use the Xbox Authentication Library (XAL) to fulfil these request by calling the
    /// <c>XalUserGetTokenAndSignatureSilentlyAsync</c> function.
    /// </remarks>
    /// <param name="correlationId">
    /// The correlation id of the web request for which the token and signature were requested.
    /// </param>
    /// <param name="succeeded">
    /// The title succeeded the token and signature request.
    /// </param>
    /// <param name="token">
    /// The token for the request, if necessary, as a null terminated string.
    /// </param>
    /// <param name="signature">
    /// The signature for the request, if necessary, as a null terminated string.
    /// </param>
    /// <remarks>
    /// This should only be called in response to a <see cref="PartyXblTokenAndSignatureRequestedStateChange" />.
    /// </remarks>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblTokenAndSignatureRequestedStateChange" />
    PartyError CompleteGetTokenAndSignatureRequest(
        uint32_t correlationId,
        PartyBool succeeded,
        _In_opt_ PartyString token,
        _In_opt_ PartyString signature
        ) party_no_throw;

    /// <summary>
    /// Queues an asynchronous attempt to create a local chat user. Privacy and privilege information related to this
    /// user will be tracked by the helper library.
    /// </summary>
    /// <param name="xboxUserId">
    /// The Xbox Live User Id associated with the local chat user.
    /// </param>
    /// <param name="asyncIdentifier">
    /// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change
    /// with this call.
    /// </param>
    /// <param name="localChatUser">
    /// The ouput local chat user object.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    PartyError CreateLocalChatUser(
        uint64_t xboxUserId,
        _In_opt_ void * asyncIdentifier,
        _Outptr_opt_ PartyXblLocalChatUser ** localChatUser
        ) party_no_throw;

    /// <summary>
    /// Creates a remote chat user. A remote chat user is an Xbox Live user currently communicating with at least one
    /// local user. Creating a remote chat user will cause the library to start tracking privacy information related to
    /// this user in relation to every local chat user.
    /// </summary>
    /// <param name="xboxUserId">
    /// The Xbox Live User Id associated with the chat user.
    /// </param>
    /// <param name="chatUser">
    /// The output chat user object.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    PartyError CreateRemoteChatUser(
        uint64_t xboxUserId,
        _Outptr_opt_ PartyXblChatUser ** chatUser
        ) party_no_throw;

    /// <summary>
    /// Destroys a chat user.
    /// </summary>
    /// <param name="chatUser">
    /// The chat user object to be destroyed.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    PartyError DestroyChatUser(
        const PartyXblChatUser * chatUser
        ) party_no_throw;

    /// <summary>
    /// Gets an array containing all chat users created by <see cref="CreateLocalChatUser()" /> and
    /// <see cref="CreateRemoteChatUser()" />.
    /// </summary>
    /// <remarks>
    /// The array is backed by the library's internal memory. The array is only valid until the next call to either
    /// <see cref="CreateLocalChatUser()" />, <see cref="CreateRemoteChatUser()" />,
    /// <see cref="StartProcessingStateChanges()" /> or <see cref="DestroyChatUser()" />. A call to DestroyChatUser()
    /// will also immediately invalidate the chat user object.
    /// </remarks>
    /// <param name="userCount">
    /// An output value indicating the number of chat users provided in <paramref name="chatUsers" />.
    /// </param>
    /// <param name="chatUsers">
    /// A library-allocated output array containing the local and remote chat users.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the call succeeded or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblManager::CreateLocalChatUser" />
    /// <seealso cref="PartyXblManager::CreateRemoteChatUser" />
    /// <seealso cref="PartyXblManager::DestroyChatUser" />
    PartyError GetChatUsers(
        _Out_ uint32_t * userCount,
        _Outptr_result_buffer_(*endpointCount) PartyXblChatUserArray * chatUsers
        ) const party_no_throw;

    /// <summary>
    /// Signs the user in to PlayFab using an Xbox Live token, asynchronously returning an Entity Id and Entity Token
    /// that can subsequently be used for PlayFab API calls which require an authenticated PlayFab user or to create a
    /// PartyLocalUser. There is no difference between calling this method or using the LoginWithXbox method provided by
    /// the PlayFab SDK.
    /// </summary>
    /// <remarks>
    /// If this is the first time the Xbox Live identity has logged in to PlayFab, a new PlayFab account will be created
    /// and linked to the Xbox Live identity.
    /// <para>
    /// When a token is nearing the expiration time or if a token has expired due to the application being in a dormant
    /// state for an extended time, a new token should be obtained by calling this method. It is recommended to acquire
    /// a new token when the previously supplied token is halfway through its validity period.
    /// </para>
    /// </remarks>
    /// <param name="localChatUser">
    /// The local chat user object that will be used for the login operation.
    /// </param>
    /// <param name="asyncIdentifier">
    /// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change
    /// with this call.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the operation to login the local chat user with PlayFab started or an error code
    /// otherwise. The human-readable form of the error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblLoginToPlayFabCompletedStateChange" />
    PartyError LoginToPlayFab(
        const PartyXblLocalChatUser * localChatUser,
        _In_opt_ void * asyncIdentifier
        ) const party_no_throw;

    /// <summary>
    /// Queues an asynchronous operation to retrieve a list of mappings between PlayFab Entity Ids and known Xbox Live
    /// User Ids.
    /// </summary>
    /// <remarks>
    /// Each Xbox Live User Id will only map to a PlayFab Entity Id if this Xbox Live User has already been linked to a
    /// PlayFab account. A PlayFab account is automatically created and linked when calling
    /// <see cref="PartyXblManager::LoginToPlayFab" /> for the first time. The <b>LoginWithXbox</b> API provided by the
    /// PlayFab SDK can also be used to link an account.
    /// <para>
    /// If the <see cref="PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange" /> associated with the completion
    /// of this call indicates a failure with a result of
    /// <see cref="PartyXblStateChangeResult::PlayFabUserNotAuthorized" />, the operation can be retried no more than
    /// one time after successfully calling LoginToPlayFab().
    /// </para>
    /// <para>
    /// If the <see cref="PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange" /> associated with the completion
    /// of this call indicates a failure with a result of
    /// <see cref="PartyXblStateChangeResult::PlayFabRateLimitExceeded" />, the operation can be retried after a brief
    /// delay.
    /// </para>
    /// </remarks>
    /// <param name="xboxLiveUserIdCount">
    /// The number of Xbox Live User Ids in the <paramref name="xboxLiveUserIds" /> array.
    /// </param>
    /// <param name="xboxLiveUserIds">
    /// An array of size <paramref name="xboxLiveUserIdCount" /> containing the Xbox Live User Ids for which the library
    /// will query the PlayFab Entity Id.
    /// </param>
    /// <param name="localChatUser">
    /// Local chat user that will be used to communicate with PlayFab. The local chat user will be logged in to PlayFab
    /// if needed.
    /// </param>
    /// <param name="asyncIdentifier">
    /// An optional, app-defined, pointer-sized context value that can be used to associate the completion state change
    /// with this call.
    /// </param>
    /// <returns>
    /// <c>c_partyErrorSuccess</c> if the operation started or an error code otherwise. The human-readable form of the
    /// error code can be retrieved via <see cref="GetErrorMessage()" />.
    /// </returns>
    /// <seealso cref="PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange" />
    PartyError GetEntityIdsFromXboxLiveUserIds(
        uint32_t xboxLiveUserIdCount,
        _In_reads_(xboxLiveUserIdCount) const uint64_t * xboxLiveUserIds,
        const PartyXblLocalChatUser * localChatUser,
        _In_opt_ void * asyncIdentifier
        ) const party_no_throw;

private:
    PartyXblManager();
    ~PartyXblManager();
    PartyXblManager(const PartyXblManager&) = delete;
    PartyXblManager(PartyXblManager&&) = delete;
    PartyXblManager& operator=(const PartyXblManager&) = delete;
    PartyXblManager& operator=(PartyXblManager&&) = delete;
    void * _handle;
};

}; // Party
