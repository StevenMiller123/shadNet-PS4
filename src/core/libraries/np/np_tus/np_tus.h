// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpTus {

enum OrbisNpTusOpeType : s32 {
    ORBIS_NP_TUS_OPETYPE_EQUAL = 1,
    ORBIS_NP_TUS_OPETYPE_NOT_EQUAL = 2,
    ORBIS_NP_TUS_OPETYPE_GREATER_THAN = 3,
    ORBIS_NP_TUS_OPETYPE_GREATER_OR_EQUAL = 4,
    ORBIS_NP_TUS_OPETYPE_LESS_THAN = 5,
    ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL = 6,
};

enum OrbisNpTusDataStatusSortType : s32 {
    ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE = 1,
    ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE = 2,
};

enum OrbisNpTusVariableSortType : s32 {
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE = 1,
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_DATE = 2,
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_VALUE = 3,
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE = 4,
};

struct OrbisNpTusVirtualUserId {
    char data[ORBIS_NP_ONLINEID_MAX_LENGTH];
    char term;
    char dummy[3];
};

enum class OrbisNpTssStatus : s32 {
    Ok = 0,
    Partial = 1,
    NotModified = 2,
};

struct OrbisNpTssDataStatus {
    Rtc::OrbisRtcTick modified;
    OrbisNpTssStatus status;
    u64 content_length;
};

enum class OrbisNpTssIfType : s32 {
    IfModifiedSince = 0,
    IfRange = 1,
};

struct OrbisNpTssIfModifiedSinceParam {
    OrbisNpTssIfType if_type;
    s8 padding[4];
    Rtc::OrbisRtcTick last_modified;
};

struct OrbisNpTssGetDataOptParam {
    u64 size;
    u64* offset;
    u64* last_byte;
    OrbisNpTssIfModifiedSinceParam* if_param;
};

struct OrbisNpTusGetFriendsVariableOptParam {
    u64 size;
    u32* start_serial_rank;
    u32* hits;
};

struct OrbisNpTusGetFriendsDataStatusOptParam {
    u64 size;
    u32* start_serial_rank;
    u32* hits;
};

struct OrbisNpTusDataInfo {
    u64 size;
    u8 data[384];
};

struct OrbisNpTusDataStatus {
    OrbisNpId owner_id;
    s32 has_data;
    Rtc::OrbisRtcTick last_changed_date;
    OrbisNpId last_changed_author_id;
    u8 pad[4];
    void* data;
    u64 data_size;
    OrbisNpTusDataInfo info;
};

struct OrbisNpTusDataStatusA {
    OrbisNpOnlineId owner_id;
    u8 reserved1[16];
    s32 has_data;
    Rtc::OrbisRtcTick last_changed_date;
    OrbisNpOnlineId last_changed_author_id;
    u8 reserved2[20];
    void* data;
    u64 data_size;
    OrbisNpTusDataInfo info;
    OrbisNpAccountId owner_account_id;
    OrbisNpAccountId last_changed_author_account_id;
    u8 reserved[16];
};

#pragma pack(push, 4)
struct OrbisNpTusVariable {
    OrbisNpId owner_id;
    s32 has_data;
    u8 pad[4];
    Rtc::OrbisRtcTick last_changed_date;
    OrbisNpId last_changed_author_id;
    s64 variable;
    s64 old_variable;
    OrbisNpAccountId owner_account_id;
    OrbisNpAccountId last_changed_author_account_id;
};

struct OrbisNpTusVariableA {
    OrbisNpOnlineId owner_id;
    u8 reserved1[16];
    s32 has_data;
    Rtc::OrbisRtcTick last_changed_date;
    u8 pad[4];
    OrbisNpOnlineId last_changed_author_id;
    u8 reserved2[16];
    s64 variable;
    s64 old_variable;
    OrbisNpAccountId owner_account_id;
    OrbisNpAccountId last_changed_author_account_id;
};
#pragma pack(pop)

struct OrbisNpTusVariableForCrossSave {
    OrbisNpId owner_id;
    s32 has_data;
    Rtc::OrbisRtcTick last_changed_date;
    u8 pad[4];
    OrbisNpId last_changed_author_id;
    s64 variable;
    s64 old_variable;
    OrbisNpAccountId owner_account_id;
    OrbisNpAccountId last_changed_author_account_id;
};

struct OrbisNpTusDataStatusForCrossSave {
    OrbisNpId owner_id;
    s32 has_data;
    Rtc::OrbisRtcTick last_changed_date;
    OrbisNpId last_changed_author_id;
    u8 pad[4];
    void* data;
    u64 data_size;
    OrbisNpTusDataInfo info;
    OrbisNpAccountId owner_account_id;
    OrbisNpAccountId last_changed_author_account_id;
    u8 reserved[16];
};

s32 sceNpTssCreateNpTitleCtx(OrbisNpServiceLabel service_label, OrbisNpId* np_id);
s32 sceNpTssCreateNpTitleCtxA(OrbisNpServiceLabel service_label,
                              UserService::OrbisUserServiceUserId user_id);
s32 sceNpTssGetData(s32 req_id, s32 slot_id, OrbisNpTssDataStatus* data_status,
                    u64 data_status_size, void* data, u64 recv_size,
                    OrbisNpTssGetDataOptParam* option);
s32 sceNpTssGetDataAsync(s32 req_id, s32 slot_id, OrbisNpTssDataStatus* data_status,
                         u64 data_status_size, void* data, u64 data_size,
                         OrbisNpTssGetDataOptParam* option);
s32 sceNpTssGetSmallStorage(s32 req_id, void* data, u64 max_size, u64* content_length,
                            void* option);
s32 sceNpTssGetSmallStorageAsync(s32 req_id, void* data, u64 max_size, u64* content_length,
                                 void* option);
s32 sceNpTssGetStorage();
s32 sceNpTssGetStorageAsync(s32 req_id);
s32 sceNpTusAbortRequest(s32 req_id);
s32 sceNpTusAddAndGetVariable(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                              s64 in_variable, const OrbisNpId* is_last_changed_author,
                              const Rtc::OrbisRtcTick* is_last_changed_date,
                              OrbisNpTusVariable* out_variable, u64 out_variable_size,
                              void* option);
s32 sceNpTusAddAndGetVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                               s64 in_variable, const OrbisNpAccountId* is_last_changed_author,
                               const Rtc::OrbisRtcTick* is_last_changed_date,
                               OrbisNpTusVariableA* out_variable, u64 out_variable_size,
                               void* option);
s32 sceNpTusAddAndGetVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                    s64 in_variable, const OrbisNpAccountId* is_last_changed_author,
                                    const Rtc::OrbisRtcTick* is_last_changed_date,
                                    OrbisNpTusVariableA* out_variable, u64 out_variable_size,
                                    void* option);
s32 sceNpTusAddAndGetVariableAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                                   s64 in_variable, const OrbisNpId* is_last_changed_author,
                                   const Rtc::OrbisRtcTick* is_last_changed_date,
                                   OrbisNpTusVariable* out_variable, u64 out_variable_size,
                                   void* option);
s32 sceNpTusAddAndGetVariableAVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableA* out_variable, u64 out_variable_size, void* option);
s32 sceNpTusAddAndGetVariableAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableA* out_variable, u64 out_variable_size, void* option);
s32 sceNpTusAddAndGetVariableForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                          s32 slot_id, s64 in_variable,
                                          const OrbisNpAccountId* is_last_changed_author,
                                          const Rtc::OrbisRtcTick* is_last_changed_date,
                                          OrbisNpTusVariableForCrossSave* out_variable,
                                          u64 out_variable_size, void* option);
s32 sceNpTusAddAndGetVariableForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                               s32 slot_id, s64 in_variable,
                                               const OrbisNpAccountId* is_last_changed_author,
                                               const Rtc::OrbisRtcTick* is_last_changed_date,
                                               OrbisNpTusVariableForCrossSave* out_variable,
                                               u64 out_variable_size, void* option);
s32 sceNpTusAddAndGetVariableForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableForCrossSave* out_variable, u64 out_variable_size, void* option);
s32 sceNpTusAddAndGetVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableForCrossSave* out_variable, u64 out_variable_size, void* option);
s32 sceNpTusAddAndGetVariableVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariable* out_variable, u64 out_variable_size, void* option);
s32 sceNpTusAddAndGetVariableVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariable* out_variable, u64 out_variable_size, void* option);
s32 sceNpTusChangeModeForOtherSaveDataOwners();
s32 sceNpTusCreateNpTitleCtx(OrbisNpServiceLabel service_label, OrbisNpId* np_id);
s32 sceNpTusCreateNpTitleCtxA(OrbisNpServiceLabel service_label,
                              UserService::OrbisUserServiceUserId user_id);
s32 sceNpTusCreateRequest(s32 lib_ctx_id);
s32 sceNpTusCreateTitleCtx();
s32 sceNpTusDeleteMultiSlotData(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotDataA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                 s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotDataAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                      s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotDataAsync(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                     s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotDataVUser(s32 req_id,
                                     const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                     s32* slot_ids, s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotDataVUserAsync(s32 req_id,
                                          const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                          s32* slot_ids, s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotVariable(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                    s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                     s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                          s32* slot_ids, s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotVariableAsync(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                         s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotVariableVUser(s32 req_id,
                                         const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                         s32* slot_ids, s32 array_len, void* option);
s32 sceNpTusDeleteMultiSlotVariableVUserAsync(s32 req_id,
                                              const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                              s32* slot_ids, s32 array_len, void* option);
s32 sceNpTusDeleteNpTitleCtx(s32 ctx_id);
s32 sceNpTusDeleteRequest(s32 request_id);
s32 sceNpTusGetData(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                    OrbisNpTusDataStatus* data_status, u64 data_status_size, void* data,
                    u64 recv_size, void* option);
s32 sceNpTusGetDataA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                     OrbisNpTusDataStatusA* data_status, u64 data_status_size, void* data,
                     u64 recv_size, void* option);
s32 sceNpTusGetDataAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                          OrbisNpTusDataStatusA* data_status, u64 data_status_size, void* data,
                          u64 recv_size, void* option);
s32 sceNpTusGetDataAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                         OrbisNpTusDataStatus* data_status, u64 data_status_size, void* data,
                         u64 recv_size, void* option);
s32 sceNpTusGetDataAVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                          s32 slot_id, OrbisNpTusDataStatusA* data_status, u64 data_status_size,
                          void* data, u64 recv_size, void* option);
s32 sceNpTusGetDataAVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                               s32 slot_id, OrbisNpTusDataStatusA* data_status,
                               u64 data_status_size, void* data, u64 recv_size, void* option);
s32 sceNpTusGetDataForCrossSave(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                OrbisNpTusDataStatusForCrossSave* data_status, u64 data_status_size,
                                void* data, u64 recv_size, void* option);
s32 sceNpTusGetDataForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                     OrbisNpTusDataStatusForCrossSave* data_status,
                                     u64 data_status_size, void* data, u64 recv_size, void* option);
s32 sceNpTusGetDataForCrossSaveVUser(s32 req_id,
                                     const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                     s32 slot_id, OrbisNpTusDataStatusForCrossSave* data_status,
                                     u64 data_status_size, void* data, u64 recv_size, void* option);
s32 sceNpTusGetDataForCrossSaveVUserAsync(s32 req_id,
                                          const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                          s32 slot_id,
                                          OrbisNpTusDataStatusForCrossSave* data_status,
                                          u64 data_status_size, void* data, u64 recv_size,
                                          void* option);
s32 sceNpTusGetDataVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                         s32 slot_id, OrbisNpTusDataStatus* data_status, u64 data_status_size,
                         void* data, u64 recv_size, void* option);
s32 sceNpTusGetDataVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                              s32 slot_id, OrbisNpTusDataStatus* data_status, u64 data_status_size,
                              void* data, u64 recv_size, void* option);
s32 sceNpTusGetFriendsDataStatus(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                 OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                 s32 array_len, void* option);
s32 sceNpTusGetFriendsDataStatusA(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                  OrbisNpTusDataStatusA* status_array, u64 status_array_size,
                                  s32 array_len, OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 sceNpTusGetFriendsDataStatusAAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                       OrbisNpTusDataStatusA* status_array, u64 status_array_size,
                                       s32 array_len,
                                       OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 sceNpTusGetFriendsDataStatusAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                      OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                      s32 array_len, void* option);
s32 sceNpTusGetFriendsDataStatusForCrossSave(s32 req_id, s32 slot_id, s32 include_self,
                                             s32 sort_type,
                                             OrbisNpTusDataStatusForCrossSave* status_array,
                                             u64 status_array_size, s32 array_len,
                                             OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 sceNpTusGetFriendsDataStatusForCrossSaveAsync(s32 req_id, s32 slot_id, s32 include_self,
                                                  s32 sort_type,
                                                  OrbisNpTusDataStatusForCrossSave* status_array,
                                                  u64 status_array_size, s32 array_len,
                                                  OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 sceNpTusGetFriendsVariable(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                               OrbisNpTusVariable* variable_array, u64 variable_array_size,
                               s32 array_len, void* option);
s32 sceNpTusGetFriendsVariableA(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                OrbisNpTusVariableA* variable_array, u64 variable_array_size,
                                s32 array_len, OrbisNpTusGetFriendsVariableOptParam* option);
s32 sceNpTusGetFriendsVariableAAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                     OrbisNpTusVariableA* variable_array, u64 variable_array_size,
                                     s32 array_len, OrbisNpTusGetFriendsVariableOptParam* option);
s32 sceNpTusGetFriendsVariableAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                    OrbisNpTusVariable* variable_array, u64 variable_array_size,
                                    s32 array_len, void* option);
s32 sceNpTusGetFriendsVariableForCrossSave(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                           OrbisNpTusVariableForCrossSave* variable_array,
                                           u64 variable_array_size, s32 array_len,
                                           OrbisNpTusGetFriendsVariableOptParam* option);
s32 sceNpTusGetFriendsVariableForCrossSaveAsync(s32 req_id, s32 slot_id, s32 include_self,
                                                s32 sort_type,
                                                OrbisNpTusVariableForCrossSave* variable_array,
                                                u64 variable_array_size, s32 array_len,
                                                OrbisNpTusGetFriendsVariableOptParam* option);
s32 sceNpTusGetMultiSlotDataStatus(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                   OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                   s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                    OrbisNpTusDataStatusA* status_array, u64 status_array_size,
                                    s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                         s32* slot_ids, OrbisNpTusDataStatusA* status_array,
                                         u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusAsync(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                        OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                        s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusAVUser(s32 req_id,
                                         const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                         s32* slot_ids, OrbisNpTusDataStatusA* status_array,
                                         u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusAVUserAsync(s32 req_id,
                                              const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                              s32* slot_ids, OrbisNpTusDataStatusA* status_array,
                                              u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                               s32* slot_ids,
                                               OrbisNpTusDataStatusForCrossSave* status_array,
                                               u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                                    s32* slot_ids,
                                                    OrbisNpTusDataStatusForCrossSave* status_array,
                                                    u64 status_array_size, s32 array_len,
                                                    void* option);
s32 sceNpTusGetMultiSlotDataStatusForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32* slot_ids,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option);
s32 sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32* slot_ids,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option);
s32 sceNpTusGetMultiSlotDataStatusVUser(s32 req_id,
                                        const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                        s32* slot_ids, OrbisNpTusDataStatus* status_array,
                                        u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotDataStatusVUserAsync(s32 req_id,
                                             const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                             s32* slot_ids, OrbisNpTusDataStatus* status_array,
                                             u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariable(s32 req_id, OrbisNpId* np_id, s32* slot_ids,
                                 OrbisNpTusVariable* variable_array, u64 variables_size,
                                 s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                  OrbisNpTusVariableA* variable_array, u64 variables_size,
                                  s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                       s32* slot_ids, OrbisNpTusVariableA* variable_array,
                                       u64 variables_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableAsync(s32 req_id, OrbisNpId* np_id, s32* slot_ids,
                                      OrbisNpTusVariable* variable_array, u64 variables_size,
                                      s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableAVUser(s32 req_id,
                                       const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                       s32* slot_ids, OrbisNpTusVariableA* variables,
                                       u64 variables_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableAVUserAsync(s32 req_id,
                                            const OrbisNpTusVirtualUserId* virtual_user_id,
                                            s32* slot_ids, OrbisNpTusVariableA* variable_array,
                                            u64 variables_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                             s32* slot_ids,
                                             OrbisNpTusVariableForCrossSave* variable_array,
                                             u64 variables_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                                  s32* slot_ids,
                                                  OrbisNpTusVariableForCrossSave* variable_array,
                                                  u64 variables_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableForCrossSaveVUser(s32 req_id,
                                                  const OrbisNpTusVirtualUserId* virtual_user_id,
                                                  s32* slot_ids,
                                                  OrbisNpTusVariableForCrossSave* variable_array,
                                                  u64 variables_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* virtual_user_id, s32* slot_ids,
    OrbisNpTusVariableForCrossSave* variable_array, u64 variables_size, s32 array_len,
    void* option);
s32 sceNpTusGetMultiSlotVariableVUser(s32 req_id,
                                      const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                      s32* slot_ids, OrbisNpTusVariable* variable_array,
                                      u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiSlotVariableVUserAsync(s32 req_id,
                                           const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                           s32* slot_ids, OrbisNpTusVariable* variable_array,
                                           u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatus(s32 req_id, const OrbisNpId* target_np_id_array, s32 slot_id,
                                   OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                   s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusA(s32 req_id, const OrbisNpAccountId* target_account_id_array,
                                    s32 slot_id, OrbisNpTusDataStatusA* status_array,
                                    u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusAAsync(s32 req_id,
                                         const OrbisNpAccountId* target_account_id_array,
                                         s32 slot_id, OrbisNpTusDataStatusA* status_array,
                                         u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusAsync(s32 req_id, const OrbisNpId* target_np_id_array,
                                        s32 slot_id, OrbisNpTusDataStatus* status_array,
                                        u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusAVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusA* status_array, u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusA* status_array, u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusForCrossSave(s32 req_id,
                                               const OrbisNpAccountId* target_account_id_array,
                                               s32 slot_id,
                                               OrbisNpTusDataStatusForCrossSave* status_array,
                                               u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusForCrossSaveAsync(s32 req_id,
                                                    const OrbisNpAccountId* target_account_id_array,
                                                    s32 slot_id,
                                                    OrbisNpTusDataStatusForCrossSave* status_array,
                                                    u64 status_array_size, s32 array_len,
                                                    void* option);
s32 sceNpTusGetMultiUserDataStatusForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option);
s32 sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option);
s32 sceNpTusGetMultiUserDataStatusVUser(s32 req_id,
                                        const OrbisNpTusVirtualUserId* target_virtual_user_id_array,
                                        s32 slot_id, OrbisNpTusDataStatus* status_array,
                                        u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserDataStatusVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatus* status_array, u64 status_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariable(s32 req_id, const OrbisNpId* target_np_id_array, s32 slot_id,
                                 OrbisNpTusVariable* variable_array, u64 variable_array_size,
                                 s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableA(s32 req_id, const OrbisNpAccountId* target_account_id_array,
                                  s32 slot_id, OrbisNpTusVariableA* variable_array,
                                  u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableAAsync(s32 req_id, const OrbisNpAccountId* target_account_id_array,
                                       s32 slot_id, OrbisNpTusVariableA* variable_array,
                                       u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableAsync(s32 req_id, const OrbisNpId* target_np_id_array, s32 slot_id,
                                      OrbisNpTusVariable* variable_array, u64 variable_array_size,
                                      s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableAVUser(s32 req_id,
                                       const OrbisNpTusVirtualUserId* target_virtual_user_id_array,
                                       s32 slot_id, OrbisNpTusVariableA* variable_array,
                                       u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariableA* variable_array, u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableForCrossSave(s32 req_id,
                                             const OrbisNpAccountId* target_account_id_array,
                                             s32 slot_id,
                                             OrbisNpTusVariableForCrossSave* variable_array,
                                             u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableForCrossSaveAsync(s32 req_id,
                                                  const OrbisNpAccountId* target_account_id_array,
                                                  s32 slot_id,
                                                  OrbisNpTusVariableForCrossSave* variable_array,
                                                  u64 variable_array_size, s32 array_len,
                                                  void* option);
s32 sceNpTusGetMultiUserVariableForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariableForCrossSave* variable_array, u64 variable_array_size, s32 array_len,
    void* option);
s32 sceNpTusGetMultiUserVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariableForCrossSave* variable_array, u64 variable_array_size, s32 array_len,
    void* option);
s32 sceNpTusGetMultiUserVariableVUser(s32 req_id,
                                      const OrbisNpTusVirtualUserId* target_virtual_user_id_array,
                                      s32 slot_id, OrbisNpTusVariable* variable_array,
                                      u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusGetMultiUserVariableVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariable* variable_array, u64 variable_array_size, s32 array_len, void* option);
s32 sceNpTusPollAsync(s32 req_id, s32* result);
s32 sceNpTusSetData(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id, u64 total_size,
                    u64 send_size, const void* data, const OrbisNpTusDataInfo* info,
                    u64 info_struct_size, const OrbisNpId* is_last_changed_author,
                    const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetDataA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id, u64 total_size,
                     u64 send_size, const void* data, const OrbisNpTusDataInfo* info,
                     u64 info_struct_size, const OrbisNpAccountId* is_last_changed_author,
                     const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetDataAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                          u64 total_size, u64 send_size, const void* data,
                          const OrbisNpTusDataInfo* info, u64 info_struct_size,
                          const OrbisNpAccountId* is_last_changed_author,
                          const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetDataAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id, u64 total_size,
                         u64 send_size, const void* data, const OrbisNpTusDataInfo* info,
                         u64 info_struct_size, const OrbisNpId* is_last_changed_author,
                         const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetDataAVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                          s32 slot_id, u64 total_size, u64 send_size, const void* data,
                          const OrbisNpTusDataInfo* info, u64 info_struct_size,
                          const OrbisNpAccountId* is_last_changed_author,
                          const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetDataAVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                               s32 slot_id, u64 total_size, u64 send_size, const void* data,
                               const OrbisNpTusDataInfo* info, u64 info_struct_size,
                               const OrbisNpAccountId* is_last_changed_author,
                               const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetDataVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                         s32 slot_id, u64 total_size, u64 send_size, const void* data,
                         const OrbisNpTusDataInfo* info, u64 info_struct_size,
                         const OrbisNpId* is_last_changed_author,
                         const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetDataVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                              s32 slot_id, u64 total_size, u64 send_size, const void* data,
                              const OrbisNpTusDataInfo* info, u64 info_struct_size,
                              const OrbisNpId* is_last_changed_author,
                              const Rtc::OrbisRtcTick* is_last_changed_date, void* option);
s32 sceNpTusSetMultiSlotVariable(s32 req_id, OrbisNpId* np_id, s32* slot_ids, s64* variables,
                                 s32 array_len, void* option);
s32 sceNpTusSetMultiSlotVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                  const s64* variables, s32 array_len, void* option);
s32 sceNpTusSetMultiSlotVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                       s32* slot_ids, const s64* variables, s32 array_len,
                                       void* option);
s32 sceNpTusSetMultiSlotVariableAsync(s32 req_id, OrbisNpId* np_id, s32* slot_ids, s64* variables,
                                      s32 array_len, void* option);
s32 sceNpTusSetMultiSlotVariableVUser(s32 req_id,
                                      const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                      s32* slot_ids, s64* variables, s32 array_len, void* option);
s32 sceNpTusSetMultiSlotVariableVUserAsync(s32 req_id,
                                           const OrbisNpTusVirtualUserId* virtual_user_id,
                                           s32* slot_ids, const s64* variables, s32 array_len,
                                           void* option);
s32 sceNpTusSetThreadParam();
s32 sceNpTusSetTimeout();
s32 sceNpTusTryAndSetVariable(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id, s32 ope_type,
                              s64 variable, const s64* compare_value,
                              const OrbisNpId* is_last_changed_author,
                              const Rtc::OrbisRtcTick* is_last_changed_date,
                              OrbisNpTusVariable* result_variable, u64 result_variable_size,
                              void* option);
s32 sceNpTusTryAndSetVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                               s32 ope_type, s64 variable, const s64* compare_value,
                               const OrbisNpAccountId* is_last_changed_author,
                               const Rtc::OrbisRtcTick* is_last_changed_date,
                               OrbisNpTusVariableA* result_variable, u64 result_variable_size,
                               void* option);
s32 sceNpTusTryAndSetVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                    s32 ope_type, s64 variable, const s64* compare_value,
                                    const OrbisNpAccountId* is_last_changed_author,
                                    const Rtc::OrbisRtcTick* is_last_changed_date,
                                    OrbisNpTusVariableA* result_variable, u64 result_variable_size,
                                    void* option);
s32 sceNpTusTryAndSetVariableAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                                   s32 ope_type, s64 variable, const s64* compare_value,
                                   const OrbisNpId* is_last_changed_author,
                                   const Rtc::OrbisRtcTick* is_last_changed_date,
                                   OrbisNpTusVariable* result_variable, u64 result_variable_size,
                                   void* option);
s32 sceNpTusTryAndSetVariableAVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableA* result_variable,
    u64 result_variable_size, void* option);
s32 sceNpTusTryAndSetVariableAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableA* result_variable,
    u64 result_variable_size, void* option);
s32 sceNpTusTryAndSetVariableForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                          s32 slot_id, s32 ope_type, s64 variable,
                                          const s64* compare_value,
                                          const OrbisNpAccountId* is_last_changed_author,
                                          const Rtc::OrbisRtcTick* is_last_changed_date,
                                          OrbisNpTusVariableForCrossSave* result_variable,
                                          u64 result_variable_size, void* option);
s32 sceNpTusTryAndSetVariableForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                               s32 slot_id, s32 ope_type, s64 variable,
                                               const s64* compare_value,
                                               const OrbisNpAccountId* is_last_changed_author,
                                               const Rtc::OrbisRtcTick* is_last_changed_date,
                                               OrbisNpTusVariableForCrossSave* result_variable,
                                               u64 result_variable_size, void* option);
s32 sceNpTusTryAndSetVariableForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableForCrossSave* result_variable,
    u64 result_variable_size, void* option);
s32 sceNpTusTryAndSetVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableForCrossSave* result_variable,
    u64 result_variable_size, void* option);
s32 sceNpTusTryAndSetVariableVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariable* result_variable,
    u64 result_variable_size, void* option);
s32 sceNpTusTryAndSetVariableVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariable* result_variable,
    u64 result_variable_size, void* option);
s32 sceNpTusWaitAsync(s32 req_id, s32* result);

void RegisterHooks();
} // namespace Libraries::Np::NpTus
