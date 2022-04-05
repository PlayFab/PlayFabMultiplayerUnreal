#pragma once

#include <PartyTypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PARTY_API __stdcall

typedef const struct PARTY_XBL* PARTY_XBL_HANDLE;
typedef const struct PARTY_XBL_CHAT_USER* PARTY_XBL_CHAT_USER_HANDLE;

// BEGIN GENERATED SECTION: DO NOT EDIT

typedef enum PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING
{
    PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING_ALLOWED,
    PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING_FRIENDS_ONLY,
    PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING_DISALLOWED,
} PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING;

typedef enum PARTY_XBL_CHAT_PERMISSION_MASK_REASON
{
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON_NO_RESTRICTION,
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON_DETERMINING,
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON_PRIVILEGE,
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON_PRIVACY,
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON_INVALID_TARGET_USER,
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON_XBOX_LIVE_SERVICE_ERROR,
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON_UNKNOWN_ERROR,
} PARTY_XBL_CHAT_PERMISSION_MASK_REASON;

typedef enum PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON
{
    PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_UNAUTHORIZED_LOCAL_CHAT_USER,
    PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_FAILED_TO_GET_TOKEN_AND_SIGNATURE_FOR_LOCAL_CHAT_USER,
    PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_XBOX_LIVE_SERVICE_TEMPORARILY_UNAVAILABLE,
    PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_INTERNET_CONNECTIVITY_ERROR,
    PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_XBOX_LIVE_USER_NOT_SIGNED_IN,
    PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON_UNKNOWN_ERROR,
} PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON;

typedef enum PARTY_XBL_STATE_CHANGE_RESULT
{
    PARTY_XBL_STATE_CHANGE_RESULT_SUCCEEDED,
    PARTY_XBL_STATE_CHANGE_RESULT_UNKNOWN_ERROR,
    PARTY_XBL_STATE_CHANGE_RESULT_CANCELED_BY_TITLE,
    PARTY_XBL_STATE_CHANGE_RESULT_USER_NOT_AUTHORIZED,
    PARTY_XBL_STATE_CHANGE_RESULT_LOGIN_TO_PLAYFAB_THROTTLED,
    PARTY_XBL_STATE_CHANGE_RESULT_PARTY_SERVICE_ERROR,
    PARTY_XBL_STATE_CHANGE_RESULT_XBOX_LIVE_SERVICE_TEMPORARILY_UNAVAILABLE,
    PARTY_XBL_STATE_CHANGE_RESULT_INTERNET_CONNECTIVITY_ERROR,
    PARTY_XBL_STATE_CHANGE_RESULT_PLAYFAB_RATE_LIMIT_EXCEEDED,
} PARTY_XBL_STATE_CHANGE_RESULT;

typedef enum PARTY_XBL_THREAD_ID
{
    PARTY_XBL_THREAD_ID_WEB_REQUEST,
} PARTY_XBL_THREAD_ID;

typedef enum PARTY_XBL_STATE_CHANGE_TYPE
{
    PARTY_XBL_STATE_CHANGE_TYPE_CREATE_LOCAL_CHAT_USER_COMPLETED,
    PARTY_XBL_STATE_CHANGE_TYPE_LOCAL_CHAT_USER_DESTROYED,
    PARTY_XBL_STATE_CHANGE_TYPE_REQUIRED_CHAT_PERMISSION_INFO_CHANGED,
    PARTY_XBL_STATE_CHANGE_TYPE_TOKEN_AND_SIGNATURE_REQUESTED,
    PARTY_XBL_STATE_CHANGE_TYPE_LOGIN_TO_PLAYFAB_COMPLETED,
    PARTY_XBL_STATE_CHANGE_TYPE_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED,
} PARTY_XBL_STATE_CHANGE_TYPE;

typedef struct PARTY_XBL_HTTP_HEADER
{
    PartyString name;
    PartyString value;
} PARTY_XBL_HTTP_HEADER;

typedef struct PARTY_XBL_CHAT_PERMISSION_INFO
{
    PARTY_CHAT_PERMISSION_OPTIONS chatPermissionMask;
    PARTY_XBL_CHAT_PERMISSION_MASK_REASON reason;
} PARTY_XBL_CHAT_PERMISSION_INFO;

typedef struct PARTY_XBL_ACCESSIBILITY_SETTINGS
{
    PartyBool speechToTextEnabled;
    PartyBool textToSpeechEnabled;
    char languageCode[PARTY_MAX_LANGUAGE_CODE_STRING_LENGTH + 1];
    PARTY_GENDER gender;
} PARTY_XBL_ACCESSIBILITY_SETTINGS;

typedef struct PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING
{
    uint64_t xboxLiveUserId;
    PartyString playfabEntityId;
} PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING;

typedef struct PARTY_XBL_STATE_CHANGE
{
    uint32_t /*PARTYXBLSTATECHANGETYPE*/ stateChangeType;
} PARTY_XBL_STATE_CHANGE;

typedef struct PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE
{
    PARTY_XBL_STATE_CHANGE stateChange;
    PARTY_XBL_STATE_CHANGE_RESULT result;
    PartyError errorDetail;
    void* asyncIdentifier;
    PARTY_XBL_CHAT_USER_HANDLE localChatUser;
} PARTY_XBL_CREATE_LOCAL_CHAT_USER_COMPLETED_STATE_CHANGE;

typedef struct PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE
{
    PARTY_XBL_STATE_CHANGE stateChange;
    PARTY_XBL_STATE_CHANGE_RESULT result;
    PartyError errorDetail;
    PARTY_XBL_CHAT_USER_HANDLE localChatUser;
    void* asyncIdentifier;
    PartyString entityId;
    PartyString titlePlayerEntityToken;
    time_t expirationTime;
} PARTY_XBL_LOGIN_TO_PLAYFAB_COMPLETED_STATE_CHANGE;

typedef struct PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE
{
    PARTY_XBL_STATE_CHANGE stateChange;
    PARTY_XBL_STATE_CHANGE_RESULT result;
    PartyError errorDetail;
    PartyString xboxLiveSandbox;
    PARTY_XBL_CHAT_USER_HANDLE localChatUser;
    void* asyncIdentifier;
    uint32_t entityIdMappingCount;
    _Field_size_(entityIdMappingCount) const PARTY_XBL_XBOX_USER_ID_TO_PLAYFAB_ENTITY_ID_MAPPING* entityIdMappings;
} PARTY_XBL_GET_ENTITY_IDS_FROM_XBOX_LIVE_USER_IDS_COMPLETED_STATE_CHANGE;

typedef struct PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE
{
    PARTY_XBL_STATE_CHANGE stateChange;
    PARTY_XBL_CHAT_USER_HANDLE localChatUser;
    PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_REASON reason;
    PartyError errorDetail;
} PARTY_XBL_LOCAL_CHAT_USER_DESTROYED_STATE_CHANGE;

typedef struct PARTY_XBL_REQUIRED_CHAT_PERMISSION_INFO_CHANGED_STATE_CHANGE
{
    PARTY_XBL_STATE_CHANGE stateChange;
    PARTY_XBL_CHAT_USER_HANDLE localChatUser;
    PARTY_XBL_CHAT_USER_HANDLE targetChatUser;
} PARTY_XBL_REQUIRED_CHAT_PERMISSION_INFO_CHANGED_STATE_CHANGE;

typedef struct PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE
{
    PARTY_XBL_STATE_CHANGE stateChange;
    uint32_t correlationId;
    PartyString method;
    PartyString url;
    uint32_t headerCount;
    _Field_size_(headerCount) PARTY_XBL_HTTP_HEADER* headers;
    uint32_t bodySize;
    _Field_size_bytes_(bodySize) const void* body;
    PartyBool forceRefresh;
    PartyBool allUsers;
    _Maybenull_ PARTY_XBL_CHAT_USER_HANDLE localChatUser;
} PARTY_XBL_TOKEN_AND_SIGNATURE_REQUESTED_STATE_CHANGE;

// END GENERATED SECTION

PartyError
PARTY_API
PartyXblChatUserIsLocal(
    PARTY_XBL_CHAT_USER_HANDLE handle,
    _Out_ PartyBool* isLocal
    );

PartyError
PARTY_API
PartyXblChatUserGetXboxUserId(
    PARTY_XBL_CHAT_USER_HANDLE handle,
    _Out_ uint64_t* xboxUserId
    );

PartyError
PARTY_API
PartyXblChatUserSetCustomContext(
    PARTY_XBL_CHAT_USER_HANDLE handle,
    _In_opt_ void* customContext
    );

PartyError
PARTY_API
PartyXblChatUserGetCustomContext(
    PARTY_XBL_CHAT_USER_HANDLE handle,
    _Outptr_result_maybenull_ void** customContext
    );


PartyError
PARTY_API
PartyXblLocalChatUserGetAccessibilitySettings(
    PARTY_XBL_CHAT_USER_HANDLE handle,
    _Out_ PARTY_XBL_ACCESSIBILITY_SETTINGS* settings
    );

PartyError
PARTY_API
PartyXblLocalChatUserGetRequiredChatPermissionInfo(
    PARTY_XBL_CHAT_USER_HANDLE handle,
    const PARTY_XBL_CHAT_USER_HANDLE targetChaUser,
    _Out_ PARTY_XBL_CHAT_PERMISSION_INFO* chatPermissionInfo
    );

PartyError
PARTY_API
PartyXblLocalChatUserGetCrossNetworkCommunicationPrivacySetting(
    PARTY_XBL_CHAT_USER_HANDLE handle,
    PARTY_XBL_CROSS_NETWORK_COMMUNICATION_PRIVACY_SETTING* setting
    );

PartyError
PARTY_API
PartyXblGetErrorMessage(
    PartyError error,
    _Outptr_ PartyString* errorMessage
    );

PartyError
PARTY_API
PartyXblSetMemoryCallbacks(
    _In_ PARTY_MEM_ALLOC_FUNC allocateMemoryCallback,
    _In_ PARTY_MEM_FREE_FUNC freeMemoryCallback
    );

PartyError
PARTY_API
PartyXblGetMemoryCallbacks(
    _Outptr_ PARTY_MEM_ALLOC_FUNC* allocateMemoryCallback,
    _Outptr_ PARTY_MEM_FREE_FUNC* freeMemoryCallback
    );

PartyError
PARTY_API
PartyXblSetThreadAffinityMask(
    PARTY_XBL_THREAD_ID threadId,
    uint64_t threadAffinityMask
    );

PartyError
PARTY_API
PartyXblGetThreadAffinityMask(
    PARTY_XBL_THREAD_ID threadId,
    _Out_ uint64_t* threadAffinityMask
    );

PartyError
PARTY_API
PartyXblInitialize(
    PARTY_HANDLE partyHandle,
    PartyString titleId,
    _Outptr_ PARTY_XBL_HANDLE* handle
    );

PartyError
PARTY_API
PartyXblCleanup(
    _Post_invalid_ PARTY_XBL_HANDLE handle
    );

PartyError
PARTY_API
PartyXblStartProcessingStateChanges(
    PARTY_XBL_HANDLE handle,
    _Out_ uint32_t* stateChangeCount,
    _Outptr_result_buffer_(*stateChangeCount) const PARTY_XBL_STATE_CHANGE*const** stateChanges
    );

PartyError
PARTY_API
PartyXblFinishProcessingStateChanges(
    PARTY_XBL_HANDLE handle,
    uint32_t stateChangeCount,
    _In_reads_(stateChangeCount) const PARTY_XBL_STATE_CHANGE*const* stateChanges
    );

PartyError
PARTY_API
PartyXblCompleteGetTokenAndSignatureRequest(
    PARTY_XBL_HANDLE handle,
    uint32_t correlationId,
    PartyBool succeeded,
   _In_opt_ PartyString token,
   _In_opt_ PartyString signature
    );

PartyError
PARTY_API
PartyXblCreateLocalChatUser(
    PARTY_XBL_HANDLE handle,
    uint64_t xboxUserId,
    _In_opt_ void* asyncIdentifier,
    _Outptr_opt_ PARTY_XBL_CHAT_USER_HANDLE* localXboxLiveUser
    );

PartyError
PARTY_API
PartyXblCreateRemoteChatUser(
    PARTY_XBL_HANDLE,
    uint64_t xboxUserId,
    _Outptr_opt_ PARTY_XBL_CHAT_USER_HANDLE* chatUser
    );

PartyError
PARTY_API
PartyXblDestroyChatUser(
    PARTY_XBL_HANDLE handle,
    PARTY_XBL_CHAT_USER_HANDLE chatUser
    );

PartyError
PARTY_API
PartyXblGetChatUsers(
    PARTY_XBL_HANDLE handle,
    _Out_ uint32_t* chatUserCount,
    _Outptr_result_buffer_(*chatUserCount) const PARTY_XBL_CHAT_USER_HANDLE** chatUsers
    );

PartyError
PARTY_API
PartyXblLoginToPlayFab(
    PARTY_XBL_CHAT_USER_HANDLE localChatUser,
    _In_opt_ void* asyncIdentifier
    );

PartyError
PARTY_API
PartyXblGetEntityIdsFromXboxLiveUserIds(
    PARTY_XBL_HANDLE handle,
    uint32_t xboxLiveUserIdCount,
    _In_reads_(xboxLiveUserIdCount) const uint64_t* xboxLiveUserIds,
    PARTY_XBL_CHAT_USER_HANDLE localChatUser,
    _In_opt_ void * asyncIdentifier
    );

///
/// The below compile time asserts are triggered when this library is being used with an incompatible version of
/// PlayFab Party. Please update both libraries to their latest version or consult the online documentation for more
/// information.
///
#pragma push_macro("PARTY_C_ASSERT")
#undef PARTY_C_ASSERT
#define PARTY_C_ASSERT(e) typedef char __PARTY_C_ASSERT__[(e)?1:-1]

PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_NONE == 0x0000);
PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_SEND_MICROPHONE_AUDIO == 0x0001);
PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_SEND_TEXT_TO_SPEECH_AUDIO == 0x0002);
PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_SEND_AUDIO == 0x0003);
PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_RECEIVE_MICROPHONE_AUDIO == 0x0004);
PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_RECEIVE_TEXT_TO_SPEECH_AUDIO == 0x0008);
PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_RECEIVE_AUDIO == 0x000c);
PARTY_C_ASSERT(PARTY_CHAT_PERMISSION_OPTIONS_RECEIVE_TEXT == 0x0010);

PARTY_C_ASSERT(PARTY_GENDER_NEUTRAL == 0x0000);
PARTY_C_ASSERT(PARTY_GENDER_FEMALE == 0x0001);
PARTY_C_ASSERT(PARTY_GENDER_MALE == 0x0002);

#pragma pop_macro("PARTY_C_ASSERT")

#ifdef __cplusplus
}
#endif
