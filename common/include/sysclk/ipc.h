/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#pragma once

#include <stdint.h>
#include "board.h"
#include "clock_manager.h"

#define SYSCLK_IPC_API_VERSION 4
#define SYSCLK_IPC_SERVICE_NAME "sys:clk"

enum SysClkIpcCmd
{
    SysClkIpcCmd_GetApiVersion = 0,
    SysClkIpcCmd_GetVersionString = 1,
    SysClkIpcCmd_GetCurrentContext = 2,
    SysClkIpcCmd_Exit = 3,
    SysClkIpcCmd_GetProfileCount = 4,
    SysClkIpcCmd_GetProfiles = 5,
    SysClkIpcCmd_SetProfiles = 6,
    SysClkIpcCmd_SetEnabled = 7,
    SysClkIpcCmd_SetOverride = 8,
    SysClkIpcCmd_GetConfigValues = 9,
    SysClkIpcCmd_SetConfigValues = 10,
    SysClkIpcCmd_GetFreqList = 11,
    // HOC-only commands — stock sys-clk returns error on unknown cmds, handled gracefully
    SysClkIpcCmd_GetProfileGovernors = 12,
    SysClkIpcCmd_SetProfileGovernors = 13,
};


typedef struct
{
    uint64_t tid;
    SysClkTitleProfileList profiles;
} SysClkIpc_SetProfiles_Args;

typedef struct
{
    SysClkModule module;
    uint32_t hz;
} SysClkIpc_SetOverride_Args;

typedef struct
{
    SysClkModule module;
    uint32_t maxCount;
} SysClkIpc_GetFreqList_Args;

// Packed per-profile governor values (one uint32_t per SysClkProfile).
// bits 7:0  = CPU governor state (0=DoNotOverride, 1=Disabled, 2=Enabled)
// bits 15:8 = GPU governor state
// bits 23:16 = VRR governor state
// Layout matches GovernorStatePack() in hocclk/board.h so both sides agree.
typedef struct
{
    uint32_t packed[SysClkProfile_EnumMax];
} SysClkProfileGovernorList;

typedef struct
{
    uint64_t tid;
    SysClkProfileGovernorList governors;
} SysClkIpc_SetProfileGovernors_Args;
