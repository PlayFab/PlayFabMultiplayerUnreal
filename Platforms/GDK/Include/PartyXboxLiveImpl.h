#pragma once

#include <PartyXboxLive_c.h>

#ifndef __cplusplus
#error "This header requires C++"
#endif // end if not defined __cplusplus

namespace Party {

#define PARTY_SAL_SUPPORT
#ifdef PARTY_SAL_SUPPORT
#pragma prefast(push)
#pragma prefast(disable:__WARNING_DANGEROUS_POINTERCAST, "All of the C++ structures are related to the corresponding C structures")
#endif // PARTY_SAL_SUPPORT

PartyXblManager::PartyXblManager() :
    _handle(nullptr)
{
}

PartyXblManager::~PartyXblManager()
{
}

PartyXblManager& PartyXblManager::GetSingleton() party_no_throw
{
    static PartyXblManager xboxLiveHelperSingleton;
    return xboxLiveHelperSingleton;
}

PartyError PartyXblManager::GetErrorMessage(
    PartyError error,
    _Outptr_ PartyString * errorMessage
    ) party_no_throw
{
    return PartyXblGetErrorMessage(error, errorMessage);
}

PartyError PartyXblManager::SetMemoryCallbacks(
    _In_ PartyAllocateMemoryCallback allocateMemoryCallback,
    _In_ PartyFreeMemoryCallback freeMemoryCallback
    ) party_no_throw
{
    return PartyXblSetMemoryCallbacks(allocateMemoryCallback, freeMemoryCallback);
}

PartyError PartyXblManager::GetMemoryCallbacks(
    _Out_ PartyAllocateMemoryCallback * allocateMemoryCallback,
    _Out_ PartyFreeMemoryCallback * freeMemoryCallback
    ) party_no_throw
{
    return PartyXblGetMemoryCallbacks(allocateMemoryCallback, freeMemoryCallback);
}

PartyError PartyXblManager::SetThreadAffinityMask(
    PartyXblThreadId threadId,
    uint64_t threadAffinityMask
    ) party_no_throw
{
    return PartyXblSetThreadAffinityMask(
       static_cast<PARTY_XBL_THREAD_ID>(threadId),
       threadAffinityMask);
}

PartyError PartyXblManager::GetThreadAffinityMask(
    PartyXblThreadId threadId,
    _Out_ uint64_t * threadAffinityMask
    ) party_no_throw
{
    return PartyXblGetThreadAffinityMask(
        static_cast<PARTY_XBL_THREAD_ID>(threadId),
        threadAffinityMask);
}

PartyError PartyXblChatUser::GetLocal(
    _Outptr_result_maybenull_ PartyXblLocalChatUser ** localChatUser
    ) const party_no_throw
{
    PartyError error;
    PartyBool isLocal;

    error = PartyXblChatUserIsLocal(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(this),
        &isLocal);
    if (PARTY_SUCCEEDED(error))
    {
        if (isLocal != 0)
        {
            *localChatUser = const_cast<PartyXblLocalChatUser *>(static_cast<const PartyXblLocalChatUser *>(this));
        }
        else
        {
            *localChatUser = nullptr;
        }
    }

    return error;
}

PartyError PartyXblChatUser::GetXboxUserId(
    _Out_ uint64_t * xboxUserId
    ) party_no_throw
{
    return PartyXblChatUserGetXboxUserId(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(this),
        xboxUserId);
}

PartyError PartyXblChatUser::GetCustomContext(
    _Outptr_result_maybenull_ void ** customContext
    ) const party_no_throw
{
    return PartyXblChatUserGetCustomContext(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(this),
        customContext);
}

PartyError PartyXblChatUser::SetCustomContext(
    _In_opt_ void * customContext
    ) party_no_throw
{
    return PartyXblChatUserSetCustomContext(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(this),
        customContext);
}

PartyError PartyXblLocalChatUser::GetAccessibilitySettings(
    _Out_ PartyXblAccessibilitySettings * settings
    ) party_no_throw
{
    return PartyXblLocalChatUserGetAccessibilitySettings(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(this),
        reinterpret_cast<PARTY_XBL_ACCESSIBILITY_SETTINGS*>(settings));
}

PartyError PartyXblLocalChatUser::GetRequiredChatPermissionInfo(
    const PartyXblChatUser * targetChatUser,
    _Out_ PartyXblChatPermissionInfo * chatPermissionInfo
    ) party_no_throw
{
    return PartyXblLocalChatUserGetRequiredChatPermissionInfo(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(this),
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(targetChatUser),
        reinterpret_cast<PARTY_XBL_CHAT_PERMISSION_INFO*>(chatPermissionInfo));
}

PartyError PartyXblLocalChatUser::GetCrossNetworkCommunicationPrivacySetting(
    _Out_ PartyXblCrossNetworkCommunicationPrivacySetting * setting
    ) party_no_throw
{
    return PartyXblLocalChatUserGetCrossNetworkCommunicationPrivacySetting(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(this),
        reinterpret_cast<PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING*>(setting));
}

PartyError PartyXblManager::Initialize(
    PartyString titleId
    ) party_no_throw
{
    void* newHandle = nullptr;
    PartyError error = PartyXblInitialize(
        *reinterpret_cast<PARTY_HANDLE*>(&PartyManager::GetSingleton()),
        titleId,
        const_cast<PARTY_XBL_HANDLE *>(reinterpret_cast<const PARTY_XBL_HANDLE *>(&newHandle)));
    if (PARTY_SUCCEEDED(error))
    {
        _handle = newHandle;
    }

    return error;
}

PartyError PartyXblManager::Cleanup() party_no_throw
{
    PartyError error = PartyXblCleanup(reinterpret_cast<PARTY_XBL_HANDLE>(_handle));
    _handle = nullptr;

    return error;
}

PartyError PartyXblManager::StartProcessingStateChanges(
    _Out_ uint32_t * stateChangeCount,
    _Outptr_result_buffer_(*stateChangeCount) PartyXblStateChangeArray * stateChanges
    ) party_no_throw
{
    return PartyXblStartProcessingStateChanges(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        stateChangeCount,
        reinterpret_cast<const PARTY_XBL_STATE_CHANGE * const **>(stateChanges));
}

PartyError PartyXblManager::FinishProcessingStateChanges(
    uint32_t stateChangeCount,
    _In_reads_(stateChangeCount) PartyXblStateChangeArray stateChanges
    ) party_no_throw
{
    return PartyXblFinishProcessingStateChanges(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        stateChangeCount,
        reinterpret_cast<const PARTY_XBL_STATE_CHANGE * const *>(stateChanges));
}

PartyError PartyXblManager::CompleteGetTokenAndSignatureRequest(
    uint32_t correlationId,
    PartyBool succeeded,
    _In_opt_ PartyString token,
    _In_opt_ PartyString signature
    ) party_no_throw
{
    return PartyXblCompleteGetTokenAndSignatureRequest(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        correlationId,
        succeeded,
        token,
        signature);
}

PartyError PartyXblManager::CreateLocalChatUser(
    uint64_t xboxUserId,
    _In_opt_ void * asyncIdentifier,
    _Outptr_ PartyXblLocalChatUser ** localChatUser
    ) party_no_throw
{
    return PartyXblCreateLocalChatUser(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        xboxUserId,
        asyncIdentifier,
        const_cast<PARTY_XBL_CHAT_USER_HANDLE *>(reinterpret_cast<const PARTY_XBL_CHAT_USER_HANDLE *>(localChatUser)));
}

PartyError PartyXblManager::CreateRemoteChatUser(
    uint64_t xboxUserId,
    _Outptr_ PartyXblChatUser ** chatUser
    ) party_no_throw
{
    return PartyXblCreateRemoteChatUser(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        xboxUserId,
        const_cast<PARTY_XBL_CHAT_USER_HANDLE *>(reinterpret_cast<const PARTY_XBL_CHAT_USER_HANDLE *>(chatUser)));
}

PartyError PartyXblManager::DestroyChatUser(
    const PartyXblChatUser * chatUser
    ) party_no_throw
{
    return PartyXblDestroyChatUser(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(chatUser));
}

PartyError PartyXblManager::GetChatUsers(
    _Out_ uint32_t * chatUserCount,
    _Outptr_result_buffer_(*endpointCount) PartyXblChatUserArray * chatUsers
    ) const party_no_throw
{
    return PartyXblGetChatUsers(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        chatUserCount,
        const_cast<const PARTY_XBL_CHAT_USER_HANDLE **>(reinterpret_cast<const PARTY_XBL_CHAT_USER_HANDLE *const*>(chatUsers)));
}

PartyError PartyXblManager::LoginToPlayFab(
    const PartyXblLocalChatUser * localChatUser,
    _In_opt_ void * asyncIdentifier
    ) const party_no_throw
{
    return PartyXblLoginToPlayFab(
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(localChatUser),
        asyncIdentifier);
}

PartyError PartyXblManager::GetEntityIdsFromXboxLiveUserIds(
    uint32_t xboxLiveUserIdCount,
    _In_reads_(xboxLiveUserIdCount) const uint64_t * xboxLiveUserIds,
    const PartyXblLocalChatUser * localChatUser,
    _In_opt_ void * asyncIdentifier
    ) const party_no_throw
{
    return PartyXblGetEntityIdsFromXboxLiveUserIds(
        reinterpret_cast<PARTY_XBL_HANDLE>(_handle),
        xboxLiveUserIdCount,
        xboxLiveUserIds,
        reinterpret_cast<PARTY_XBL_CHAT_USER_HANDLE>(localChatUser),
        asyncIdentifier);
}

//
// C to C++ structure and constant verification
//
#pragma push_macro("PARTY_C_ASSERT")
#undef PARTY_C_ASSERT
#define PARTY_C_ASSERT(e) typedef char __PARTY_C_ASSERT__[(e)?1:-1]

// BEGIN GENERATED SECTION: DO NOT EDIT

PARTY_C_ASSERT(PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING_ALLOWED == static_cast<uint32_t>(PartyXblCrossNetworkCommunicationPrivacySetting::Allowed));
PARTY_C_ASSERT(PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING_FRIENDS_ONLY == static_cast<uint32_t>(PartyXblCrossNetworkCommunicationPrivacySetting::FriendsOnly));
PARTY_C_ASSERT(PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING_DISALLOWED == static_cast<uint32_t>(PartyXblCrossNetworkCommunicationPrivacySetting::Disallowed));

PARTY_C_ASSERT(PARTY_XBL_CHAT_PERMISSION_MASK_REASON_NO_RESTRICTION == static_cast<uint32_t>(PartyXblChatPermissionMaskReason::NoRestriction));
PARTY_C_ASSERT(PARTY_XBL_CHAT_PERMISSION_MASK_REASON_DETERMINING == static_cast<uint32_t>(PartyXblChatPermissionMaskReason::Determining));
PARTY_C_ASSERT(PARTY_XBL_CHAT_PERMISSION_MASK_REASON_PRIVILEGE == static_cast<uint32_t>(PartyXblChatPermissionMaskReason::Privilege));
PARTY_C_ASSERT(PARTY_XBL_CHAT_PERMISSION_MASK_REASON_PRIVACY == static_cast<uint32_t>(PartyXblChatPermissionMaskReason::Privacy));
PARTY_C_ASSERT(PARTY_XBL_CHAT_PERMISSION_MASK_REASON_INVALID_TARGET_USER == static_cast<uint32_t>(PartyXblChatPermissionMaskReason::InvalidTargetUser));
PARTY_C_ASSERT(PARTY_XBL_CHAT_PERMISSION_MASK_REASON_XBOX_LIVE_SERVICE_ERROR == static_cast<uint32_t>(PartyXblChatPermissionMaskReason::XboxLiveServiceError));
PARTY_C_ASSERT(PARTY_XBL_CHAT_PERMISSION_MASK_REASON_UNKNOWN_ERROR == static_cast<uint32_t>(PartyXblChatPermissionMaskReason::UnknownError));

PARTY_C_ASSERT(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_UNAUTHORIZED_LOCAL_CHAT_USER == static_cast<uint32_t>(PartyXblLocalChatUserDestroyedReason::UnauthorizedLocalChatUser));
PARTY_C_ASSERT(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_FAILED_TO_GET_TOKEN_AND_SIGNATURE_FOR_LOCAL_CHAT_USER == static_cast<uint32_t>(PartyXblLocalChatUserDestroyedReason::FailedToGetTokenAndSignatureForLocalChatUser));
PARTY_C_ASSERT(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_XBOX_LIVE_SERVICE_TEMPORARILY_UNAVAILABLE == static_cast<uint32_t>(PartyXblLocalChatUserDestroyedReason::XboxLiveServiceTemporarilyUnavailable));
PARTY_C_ASSERT(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_INTERNET_CONNECTIVITY_ERROR == static_cast<uint32_t>(PartyXblLocalChatUserDestroyedReason::InternetConnectivityError));
PARTY_C_ASSERT(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_XBOX_LIVE_USER_NOT_SIGNED_IN == static_cast<uint32_t>(PartyXblLocalChatUserDestroyedReason::XboxLiveUserNotSignedIn));
PARTY_C_ASSERT(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_UNKNOWN_ERROR == static_cast<uint32_t>(PartyXblLocalChatUserDestroyedReason::UnknownError));

PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_SUCCEEDED == static_cast<uint32_t>(PartyXblStateChangeResult::Succeeded));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_UNKNOWN_ERROR == static_cast<uint32_t>(PartyXblStateChangeResult::UnknownError));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_CANCELED_BY_TITLE == static_cast<uint32_t>(PartyXblStateChangeResult::CanceledByTitle));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_USER_NOT_AUTHORIZED == static_cast<uint32_t>(PartyXblStateChangeResult::UserNotAuthorized));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_LOGIN_TO_PLAYFAB_THROTTLED == static_cast<uint32_t>(PartyXblStateChangeResult::LoginToPlayFabThrottled));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_PARTY_SERVICE_ERROR == static_cast<uint32_t>(PartyXblStateChangeResult::PartyServiceError));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_XBOX_LIVE_SERVICE_TEMPORARILY_UNAVAILABLE == static_cast<uint32_t>(PartyXblStateChangeResult::XboxLiveServiceTemporarilyUnavailable));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_INTERNET_CONNECTIVITY_ERROR == static_cast<uint32_t>(PartyXblStateChangeResult::InternetConnectivityError));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_RESULT_PLAYFAB_RATE_LIMIT_EXCEEDED == static_cast<uint32_t>(PartyXblStateChangeResult::PlayFabRateLimitExceeded));

PARTY_C_ASSERT(PARTY_XBL_THREAD_ID_WEB_REQUEST == static_cast<uint32_t>(PartyXblThreadId::WebRequest));

PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_TYPE_CREATE_LOCAL_CHAT_USER_COMPLETED == static_cast<uint32_t>(PartyXblStateChangeType::CreateLocalChatUserCompleted));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_TYPE_LOCAL_CHAT_USER_DESTROYED == static_cast<uint32_t>(PartyXblStateChangeType::LocalChatUserDestroyed));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_TYPE_REQUIRED_CHAT_PERMISSION_INFO_CHANGED == static_cast<uint32_t>(PartyXblStateChangeType::RequiredChatPermissionInfoChanged));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_TYPE_TOKEN_AND_SIGNATURE_REQUESTED == static_cast<uint32_t>(PartyXblStateChangeType::TokenAndSignatureRequested));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_TYPE_LOGIN_TO_PLAYFAB_COMPLETED == static_cast<uint32_t>(PartyXblStateChangeType::LoginToPlayFabCompleted));
PARTY_C_ASSERT(PARTY_XBL_STATE_CHANGE_TYPE_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED == static_cast<uint32_t>(PartyXblStateChangeType::GetEntityIdsFromXboxLiveUserIdsCompleted));

PARTY_C_ASSERT(sizeof(PARTY_XBL_HTTP_HEADER) == sizeof(PartyXblHttpHeader));
PARTY_C_ASSERT(sizeof(PARTY_XBL_HTTP_HEADER::name) == sizeof(PartyXblHttpHeader::name));
PARTY_C_ASSERT(offsetof(PARTY_XBL_HTTP_HEADER, name) == offsetof(PartyXblHttpHeader, name));
PARTY_C_ASSERT(sizeof(PARTY_XBL_HTTP_HEADER::value) == sizeof(PartyXblHttpHeader::value));
PARTY_C_ASSERT(offsetof(PARTY_XBL_HTTP_HEADER, value) == offsetof(PartyXblHttpHeader, value));

PARTY_C_ASSERT(sizeof(PARTY_XBL_CHAT_PERMISSION_INFO) == sizeof(PartyXblChatPermissionInfo));
PARTY_C_ASSERT(sizeof(PARTY_XBL_CHAT_PERMISSION_INFO::chatPermissionMask) == sizeof(PartyXblChatPermissionInfo::chatPermissionMask));
PARTY_C_ASSERT(offsetof(PARTY_XBL_CHAT_PERMISSION_INFO, chatPermissionMask) == offsetof(PartyXblChatPermissionInfo, chatPermissionMask));
PARTY_C_ASSERT(sizeof(PARTY_XBL_CHAT_PERMISSION_INFO::reason) == sizeof(PartyXblChatPermissionInfo::reason));
PARTY_C_ASSERT(offsetof(PARTY_XBL_CHAT_PERMISSION_INFO, reason) == offsetof(PartyXblChatPermissionInfo, reason));

PARTY_C_ASSERT(sizeof(PARTY_XBL_ACCESSIBILITY_SETTINGS) == sizeof(PartyXblAccessibilitySettings));
PARTY_C_ASSERT(sizeof(PARTY_XBL_ACCESSIBILITY_SETTINGS::speechToTextEnabled) == sizeof(PartyXblAccessibilitySettings::speechToTextEnabled));
PARTY_C_ASSERT(offsetof(PARTY_XBL_ACCESSIBILITY_SETTINGS, speechToTextEnabled) == offsetof(PartyXblAccessibilitySettings, speechToTextEnabled));
PARTY_C_ASSERT(sizeof(PARTY_XBL_ACCESSIBILITY_SETTINGS::textToSpeechEnabled) == sizeof(PartyXblAccessibilitySettings::textToSpeechEnabled));
PARTY_C_ASSERT(offsetof(PARTY_XBL_ACCESSIBILITY_SETTINGS, textToSpeechEnabled) == offsetof(PartyXblAccessibilitySettings, textToSpeechEnabled));
PARTY_C_ASSERT(sizeof(PARTY_XBL_ACCESSIBILITY_SETTINGS::languageCode) == sizeof(PartyXblAccessibilitySettings::languageCode));
PARTY_C_ASSERT(offsetof(PARTY_XBL_ACCESSIBILITY_SETTINGS, languageCode) == offsetof(PartyXblAccessibilitySettings, languageCode));
PARTY_C_ASSERT(sizeof(PARTY_XBL_ACCESSIBILITY_SETTINGS::gender) == sizeof(PartyXblAccessibilitySettings::gender));
PARTY_C_ASSERT(offsetof(PARTY_XBL_ACCESSIBILITY_SETTINGS, gender) == offsetof(PartyXblAccessibilitySettings, gender));

PARTY_C_ASSERT(sizeof(PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING) == sizeof(PartyXblXboxUserIdToPlayFabEntityIdMapping));
PARTY_C_ASSERT(sizeof(PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING::xboxLiveUserId) == sizeof(PartyXblXboxUserIdToPlayFabEntityIdMapping::xboxLiveUserId));
PARTY_C_ASSERT(offsetof(PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING, xboxLiveUserId) == offsetof(PartyXblXboxUserIdToPlayFabEntityIdMapping, xboxLiveUserId));
PARTY_C_ASSERT(sizeof(PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING::playfabEntityId) == sizeof(PartyXblXboxUserIdToPlayFabEntityIdMapping::playfabEntityId));
PARTY_C_ASSERT(offsetof(PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING, playfabEntityId) == offsetof(PartyXblXboxUserIdToPlayFabEntityIdMapping, playfabEntityId));

PARTY_C_ASSERT(sizeof(PARTY_XBL_STATE_CHANGE) == sizeof(PartyXblStateChange));
PARTY_C_ASSERT(sizeof(PARTY_XBL_STATE_CHANGE::stateChangeType) == sizeof(PartyXblStateChange::stateChangeType));
PARTY_C_ASSERT(offsetof(PARTY_XBL_STATE_CHANGE, stateChangeType) == offsetof(PartyXblStateChange, stateChangeType));

PARTY_C_ASSERT(sizeof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE) == sizeof(PartyXblCreateLocalChatUserCompletedStateChange));
PARTY_C_ASSERT(sizeof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE::result) == sizeof(PartyXblCreateLocalChatUserCompletedStateChange::result));
PARTY_C_ASSERT(offsetof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE, result) == offsetof(PartyXblCreateLocalChatUserCompletedStateChange, result));
PARTY_C_ASSERT(sizeof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE::errorDetail) == sizeof(PartyXblCreateLocalChatUserCompletedStateChange::errorDetail));
PARTY_C_ASSERT(offsetof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE, errorDetail) == offsetof(PartyXblCreateLocalChatUserCompletedStateChange, errorDetail));
PARTY_C_ASSERT(sizeof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE::asyncIdentifier) == sizeof(PartyXblCreateLocalChatUserCompletedStateChange::asyncIdentifier));
PARTY_C_ASSERT(offsetof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE, asyncIdentifier) == offsetof(PartyXblCreateLocalChatUserCompletedStateChange, asyncIdentifier));
PARTY_C_ASSERT(sizeof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE::localChatUser) == sizeof(PartyXblCreateLocalChatUserCompletedStateChange::localChatUser));
PARTY_C_ASSERT(offsetof(PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE, localChatUser) == offsetof(PartyXblCreateLocalChatUserCompletedStateChange, localChatUser));

PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE) == sizeof(PartyXblLoginToPlayFabCompletedStateChange));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE::result) == sizeof(PartyXblLoginToPlayFabCompletedStateChange::result));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE, result) == offsetof(PartyXblLoginToPlayFabCompletedStateChange, result));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE::errorDetail) == sizeof(PartyXblLoginToPlayFabCompletedStateChange::errorDetail));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE, errorDetail) == offsetof(PartyXblLoginToPlayFabCompletedStateChange, errorDetail));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE::localChatUser) == sizeof(PartyXblLoginToPlayFabCompletedStateChange::localChatUser));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE, localChatUser) == offsetof(PartyXblLoginToPlayFabCompletedStateChange, localChatUser));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE::asyncIdentifier) == sizeof(PartyXblLoginToPlayFabCompletedStateChange::asyncIdentifier));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE, asyncIdentifier) == offsetof(PartyXblLoginToPlayFabCompletedStateChange, asyncIdentifier));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE::entityId) == sizeof(PartyXblLoginToPlayFabCompletedStateChange::entityId));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE, entityId) == offsetof(PartyXblLoginToPlayFabCompletedStateChange, entityId));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE::titlePlayerEntityToken) == sizeof(PartyXblLoginToPlayFabCompletedStateChange::titlePlayerEntityToken));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE, titlePlayerEntityToken) == offsetof(PartyXblLoginToPlayFabCompletedStateChange, titlePlayerEntityToken));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE::expirationTime) == sizeof(PartyXblLoginToPlayFabCompletedStateChange::expirationTime));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE, expirationTime) == offsetof(PartyXblLoginToPlayFabCompletedStateChange, expirationTime));

PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange));
PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE::result) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange::result));
PARTY_C_ASSERT(offsetof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE, result) == offsetof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange, result));
PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE::errorDetail) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange::errorDetail));
PARTY_C_ASSERT(offsetof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE, errorDetail) == offsetof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange, errorDetail));
PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE::xboxLiveSandbox) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange::xboxLiveSandbox));
PARTY_C_ASSERT(offsetof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE, xboxLiveSandbox) == offsetof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange, xboxLiveSandbox));
PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE::localChatUser) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange::localChatUser));
PARTY_C_ASSERT(offsetof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE, localChatUser) == offsetof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange, localChatUser));
PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE::asyncIdentifier) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange::asyncIdentifier));
PARTY_C_ASSERT(offsetof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE, asyncIdentifier) == offsetof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange, asyncIdentifier));
PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE::entityIdMappingCount) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange::entityIdMappingCount));
PARTY_C_ASSERT(offsetof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE, entityIdMappingCount) == offsetof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange, entityIdMappingCount));
PARTY_C_ASSERT(sizeof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE::entityIdMappings) == sizeof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange::entityIdMappings));
PARTY_C_ASSERT(offsetof(PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE, entityIdMappings) == offsetof(PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange, entityIdMappings));

PARTY_C_ASSERT(sizeof(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE) == sizeof(PartyXblLocalChatUserDestroyedStateChange));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE::localChatUser) == sizeof(PartyXblLocalChatUserDestroyedStateChange::localChatUser));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE, localChatUser) == offsetof(PartyXblLocalChatUserDestroyedStateChange, localChatUser));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE::reason) == sizeof(PartyXblLocalChatUserDestroyedStateChange::reason));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE, reason) == offsetof(PartyXblLocalChatUserDestroyedStateChange, reason));
PARTY_C_ASSERT(sizeof(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE::errorDetail) == sizeof(PartyXblLocalChatUserDestroyedStateChange::errorDetail));
PARTY_C_ASSERT(offsetof(PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE, errorDetail) == offsetof(PartyXblLocalChatUserDestroyedStateChange, errorDetail));

PARTY_C_ASSERT(sizeof(PARTY_XBL_REQUIRED_CHAT_PERMISSION_INFO_CHANGED_STATE_CHANGE) == sizeof(PartyXblRequiredChatPermissionInfoChangedStateChange));
PARTY_C_ASSERT(sizeof(PARTY_XBL_REQUIRED_CHAT_PERMISSION_INFO_CHANGED_STATE_CHANGE::localChatUser) == sizeof(PartyXblRequiredChatPermissionInfoChangedStateChange::localChatUser));
PARTY_C_ASSERT(offsetof(PARTY_XBL_REQUIRED_CHAT_PERMISSION_INFO_CHANGED_STATE_CHANGE, localChatUser) == offsetof(PartyXblRequiredChatPermissionInfoChangedStateChange, localChatUser));
PARTY_C_ASSERT(sizeof(PARTY_XBL_REQUIRED_CHAT_PERMISSION_INFO_CHANGED_STATE_CHANGE::targetChatUser) == sizeof(PartyXblRequiredChatPermissionInfoChangedStateChange::targetChatUser));
PARTY_C_ASSERT(offsetof(PARTY_XBL_REQUIRED_CHAT_PERMISSION_INFO_CHANGED_STATE_CHANGE, targetChatUser) == offsetof(PartyXblRequiredChatPermissionInfoChangedStateChange, targetChatUser));

PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE) == sizeof(PartyXblTokenAndSignatureRequestedStateChange));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::correlationId) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::correlationId));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, correlationId) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, correlationId));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::method) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::method));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, method) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, method));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::url) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::url));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, url) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, url));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::headerCount) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::headerCount));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, headerCount) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, headerCount));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::headers) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::headers));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, headers) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, headers));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::bodySize) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::bodySize));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, bodySize) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, bodySize));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::body) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::body));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, body) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, body));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::forceRefresh) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::forceRefresh));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, forceRefresh) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, forceRefresh));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::allUsers) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::allUsers));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, allUsers) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, allUsers));
PARTY_C_ASSERT(sizeof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE::localChatUser) == sizeof(PartyXblTokenAndSignatureRequestedStateChange::localChatUser));
PARTY_C_ASSERT(offsetof(PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE, localChatUser) == offsetof(PartyXblTokenAndSignatureRequestedStateChange, localChatUser));

PARTY_C_ASSERT(sizeof(PARTY_XBL_CHAT_USER_HANDLE) == sizeof(PartyXblLocalChatUser*));
PARTY_C_ASSERT(sizeof(PARTY_XBL_CHAT_USER_HANDLE) == sizeof(PartyXblChatUser*));

PARTY_C_ASSERT(sizeof(PartyXblStateChangeType) == sizeof(uint32_t));

// END GENERATED SECTION

#pragma pop_macro("PARTY_C_ASSERT")

#ifdef PARTY_SAL_SUPPORT
#pragma prefast(pop)
#endif // PARTY_SAL_SUPPORT

}; // Party
