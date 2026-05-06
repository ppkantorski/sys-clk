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

typedef struct
{
    uint8_t enabled;
    uint64_t applicationId;
    SysClkProfile profile;
    uint32_t freqs[SysClkModule_EnumMax];
    uint32_t realFreqs[SysClkModule_EnumMax];
    uint32_t overrideFreqs[SysClkModule_EnumMax];
    uint32_t temps[SysClkThermalSensor_EnumMax];
    int32_t power[SysClkPowerSensor_EnumMax];
    uint32_t ramLoad[SysClkRamLoad_EnumMax];
    uint32_t realVolts[4];
    // HOC extension: per-component die temperatures (milliCelsius).
    // CPU die, GPU die, MEM (PLLX proxy on Mariko).
    // Zero-filled when running against stock sys-clk (struct extended, old
    // sysmodule sends 104 bytes → these 12 bytes stay zeroed from the
    // zero-initialised SysClkContext the overlay allocates).
    uint32_t componentTemps[3];
    // HOC extension: temporary governor override packed value.
    // bits 7:0 = CPU governor, bits 15:8 = GPU governor.
    // (0=DoNotOverride, 1=Disabled, 2=Enabled per component)
    // Zero when running against stock sys-clk.
    uint32_t governorOverride;
} SysClkContext;

typedef struct
{
    union {
        uint32_t mhz[SysClkProfile_EnumMax * SysClkModule_EnumMax];
        uint32_t mhzMap[SysClkProfile_EnumMax][SysClkModule_EnumMax];
    };
} SysClkTitleProfileList;

#define SYSCLK_FREQ_LIST_MAX 32
