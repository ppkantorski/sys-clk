/*
 * SOCTHERM – CPU/GPU/MEM die temperatures (non-HOC path)
 *
 * Ported from Status-Monitor-Overlay by MasaGratoR / p-sam
 *
 * HOS does not enable the SENSOR_TEMP1/TEMP2 combined outputs — it reads
 * temperatures through its own internal path.  We must initialise the
 * sensors ourselves: read per-chip fuse calibration, compute therma/thermb
 * coefficients, write them to each sensor's CFG2 register, then enable
 * TSENSOR_CLKEN.  After that the hardware updates SENSOR_TEMP1/2 every
 * measurement cycle and we just read them.
 *
 * CAR is NOT touched — HOS already runs the SOCTHERM clock and we must not
 * disturb the clock controller.  Sleep detection uses TSENSOR_CLKEN readback
 * instead of the CAR registers HOC uses.
 *
 * Requires svcQueryMemoryMapping access to:
 *   0x700E2000 (SOCTHERM, 4KB) read+write
 *   0x7000F000 (FUSE,     4KB) read-only
 * (Tesla overlays run under nx-ovlloader which grants these through the
 *  standard overlay kernel capabilities — no extra NPDM entries needed.)
 */

#pragma once
#include <switch.h>

// Forward-declared by base_menu_gui.cpp (cached via IsMariko())
extern bool IsMariko();

namespace Soctherm {

// Physical base addresses
static constexpr u64 PA_SOC  = 0x700E2000;
static constexpr u64 PA_FUSE = 0x7000F000;

// SOCTHERM register offsets
static constexpr u32 CFG0             = 0x00;
static constexpr u32 CFG1             = 0x04;
static constexpr u32 CFG2             = 0x08;
static constexpr u32 CFG0_TALL_SHIFT  = 8;
static constexpr u32 CFG1_TSMP_SHIFT  = 0;
static constexpr u32 CFG1_TIDDQ_SHIFT = 15;
static constexpr u32 CFG1_TEN_SHIFT   = 24;
static constexpr u32 CFG1_TEMP_ENABLE = 1u << 31;
static constexpr u32 CFG2_THERMA_SHIFT= 16;

static constexpr u32 SENSOR_PDIV        = 0x1C0;
static constexpr u32 SENSOR_HOTSPOT_OFF = 0x1C4;
static constexpr u32 SENSOR_TEMP1       = 0x1C8; // [31:16]=CPU  [15:0]=GPU
static constexpr u32 SENSOR_TEMP2       = 0x1CC; // [31:16]=MEM  [15:0]=PLLX
static constexpr u32 TSENSOR_CLKEN      = 0x1DC;
static constexpr u32 TSENSOR_ENABLE     = 225u;

static constexpr u32 PDIV_T210B0 = 0xCC0Cu;
static constexpr u32 PDIV_T210   = 0x8888u;
static constexpr u32 HOTSPOT_VAL = 0x000A0500u;
static constexpr u32 PDIV_MASK_T210B0  = 0xFFFF00F0u;
static constexpr u32 PDIV_MASK_T210    = 0xFFFF0000u;
static constexpr u32 HSPOT_MASK_T210B0 = 0xFF0000FFu;
static constexpr u32 HSPOT_MASK_T210   = 0xFF000000u;

// READBACK format bits
static constexpr u32 RB_MASK  = 0xFF00u;
static constexpr u32 RB_SHIFT = 8;
static constexpr u32 RB_HALF  = 1u << 7;
static constexpr u32 RB_NEG   = 1u << 0;

// FUSE offsets (relative to PA_FUSE)
static constexpr u32 FUSE_CACHE_OFF    = 0x800;
static constexpr u32 FUSE_COMMON       = 0xA80;
static constexpr u32 FUSE_CP_MASK      = 0x3FFu << 11;
static constexpr u32 FUSE_CP_SHIFT     = 11;
static constexpr u32 FUSE_FT_MASK      = 0x7FFu << 21;
static constexpr u32 FUSE_FT_SHIFT     = 21;
static constexpr u32 FUSE_FT_BASE_MASK = 0x1FFFu << 13;
static constexpr u32 FUSE_FT_BASE_SHIFT= 13;
static constexpr u32 FUSE_SFT_FT_MASK  = 0x1Fu << 6;
static constexpr u32 FUSE_SFT_FT_SHIFT = 6;
static constexpr s32 NOMINAL_CP        = 25;
static constexpr s32 NOMINAL_FT        = 105;
static constexpr s64 CALIB_COEFF       = 1000000LL;

struct TSConf { u32 tall,tiddq,ten,pdiv,pdiv_ate,tsmp,tsmp_ate; };
struct FCorr  { s32 alpha, beta; };
struct TSGrp  { u32 temp_off, pdiv_mask; };
struct TSens  { u32 base; const TSConf* cfg; u32 fuse_off; FCorr corr; const TSGrp* grp; };

static constexpr TSConf eristaConf = {16300,1,1, 8, 8,120,480};
static constexpr TSConf marikoConf = {16300,1,1,12, 6,240,480};

static constexpr TSGrp gCpu = {SENSOR_TEMP1, 0xFu << 12};
static constexpr TSGrp gGpu = {SENSOR_TEMP1, 0xFu <<  8};
static constexpr TSGrp gPll = {SENSOR_TEMP2, 0xFu <<  0};
static constexpr TSGrp gMem = {SENSOR_TEMP2, 0xFu <<  4};

static constexpr TSens eS[] = {
    {0x0C0,&eristaConf,0x198,{1085000, 3244200},&gCpu},
    {0x0E0,&eristaConf,0x184,{1126200,  -67500},&gCpu},
    {0x100,&eristaConf,0x188,{1098400, 2251100},&gCpu},
    {0x120,&eristaConf,0x22C,{1108000,  602700},&gCpu},
    {0x180,&eristaConf,0x254,{1074300, 2734900},&gGpu},
    {0x1A0,&eristaConf,0x260,{1039700, 6829100},&gPll},
    {0x140,&eristaConf,0x258,{1069200, 3549900},&gMem},
    {0x160,&eristaConf,0x25C,{1173700,-6263600},&gMem},
};
static constexpr TSens mS[] = {
    {0x0C0,&marikoConf,0x198,{1085000, 3244200},&gCpu},
    {0x0E0,&marikoConf,0x184,{1126200,  -67500},&gCpu},
    {0x100,&marikoConf,0x188,{1098400, 2251100},&gCpu},
    {0x120,&marikoConf,0x22C,{1108000,  602700},&gCpu},
    {0x180,&marikoConf,0x254,{1074300, 2734900},&gGpu},
    {0x1A0,&marikoConf,0x260,{1039700, 6829100},&gPll},
};

static u64  vSoc  = 0;
static u64  vFuse = 0;
static u32  cal[8] = {};
static bool ready = false;

// Raw register helpers
template<typename T=u32> static T    Rd(u64 b, u32 o)     { return *reinterpret_cast<volatile T*>(b+o); }
template<typename T=u32> static void Wr(u64 b, u32 o, T v) { *reinterpret_cast<volatile T*>(b+o) = v; }

static bool MapPA(u64& va, u64 pa) {
    if (hosversionAtLeast(10,0,0)) {
        u64 sz = 0;
        return R_SUCCEEDED(svcQueryMemoryMapping(&va, &sz, pa, 0x1000));
    } else {
        return R_SUCCEEDED(svcLegacyQueryIoMapping(&va, pa, 0x1000));
    }
}

static s32 Sext32(u32 v, int idx) { u8 sh = 31-idx; return (s32)(v<<sh)>>sh; }
static s64 Div64p(s64 a, s32 b)   { s64 al=a<<16; return ((al*2+1)/(2*(s64)b))>>16; }

// Decode SOCTHERM READBACK format → milliCelsius (identical to HOC TranslateTemp)
static s32 Trans(u16 val) {
    s32 t = ((val & RB_MASK) >> RB_SHIFT) * 1000;
    if (val & RB_HALF) t += 500;
    if (val & RB_NEG)  t  = -t;
    return t;
}

static void CalcCal(const TSens& s, u32 bcp, u32 bft, s32 tcp, s32 tft, u32& out) {
    u32 raw  = Rd(vFuse, s.fuse_off + FUSE_CACHE_OFF);
    s32 tscp = (s32)(bcp*64) + Sext32(raw, 12);
    s32 tsft = (s32)(bft*32) + Sext32((raw & FUSE_FT_BASE_MASK) >> FUSE_FT_BASE_SHIFT, 12);
    s32 ds   = tsft - tscp,  dt = tft - tcp;
    s32 mult = s.cfg->pdiv * s.cfg->tsmp_ate;
    s32 div  = s.cfg->tsmp * s.cfg->pdiv_ate;
    s64 tmp  = (s64)dt * (1LL<<13) * mult;
    s16 A    = (s16)Div64p(tmp, (s64)ds * div);
    tmp      = (s64)tsft * tcp - (s64)tscp * tft;
    s16 B    = (s16)Div64p(tmp, ds);
    tmp      = (s64)A * s.corr.alpha;
    A        = (s16)Div64p(tmp, CALIB_COEFF);
    tmp      = (s64)B * s.corr.alpha + s.corr.beta;
    B        = (s16)Div64p(tmp, CALIB_COEFF);
    out      = ((u16)A << CFG2_THERMA_SHIFT) | (u16)B;
}

static void EnSensor(const TSens& s, u32 c) {
    Wr(vSoc, s.base + CFG0,
        s.cfg->tall << CFG0_TALL_SHIFT);
    Wr(vSoc, s.base + CFG1,
        ((s.cfg->tsmp - 1) << CFG1_TSMP_SHIFT) |
        (s.cfg->tiddq      << CFG1_TIDDQ_SHIFT) |
        (s.cfg->ten        << CFG1_TEN_SHIFT)   |
        CFG1_TEMP_ENABLE);
    Wr(vSoc, s.base + CFG2, c);
}

static void StartSensors() {
    bool mariko = IsMariko();
    if (mariko) {
        for (u32 i = 0; i < 6; ++i) EnSensor(mS[i], cal[i]);
        Wr(vSoc, SENSOR_PDIV,
            (Rd(vSoc,SENSOR_PDIV) & PDIV_MASK_T210B0) | PDIV_T210B0);
        Wr(vSoc, SENSOR_HOTSPOT_OFF,
            (Rd(vSoc,SENSOR_HOTSPOT_OFF) & HSPOT_MASK_T210B0) | HOTSPOT_VAL);
    } else {
        for (u32 i = 0; i < 8; ++i) EnSensor(eS[i], cal[i]);
        Wr(vSoc, SENSOR_PDIV,
            (Rd(vSoc,SENSOR_PDIV) & PDIV_MASK_T210) | PDIV_T210);
        Wr(vSoc, SENSOR_HOTSPOT_OFF,
            (Rd(vSoc,SENSOR_HOTSPOT_OFF) & HSPOT_MASK_T210) | HOTSPOT_VAL);
    }
    Wr(vSoc, TSENSOR_CLKEN, TSENSOR_ENABLE);
}

// One-time init: map hardware, compute fuse calibration, enable sensors.
// Safe to call multiple times (guarded by `ready`).
static void Initialize() {
    if (ready) return;
    if (!MapPA(vSoc,  PA_SOC))  return;
    if (!MapPA(vFuse, PA_FUSE)) { vSoc = 0; return; }

    // HOS clock is already running — no CAR writes needed.
    u32 fc  = Rd(vFuse, FUSE_COMMON);
    u32 bcp = (fc & FUSE_CP_MASK)      >> FUSE_CP_SHIFT;
    u32 bft = (fc & FUSE_FT_MASK)      >> FUSE_FT_SHIFT;
    s32 sft = Sext32((fc & FUSE_SFT_FT_MASK) >> FUSE_SFT_FT_SHIFT, 4);
    s32 scp = Sext32(fc, 5);
    s32 tcp = 2 * NOMINAL_CP + scp;
    s32 tft = 2 * NOMINAL_FT + sft;

    bool mariko = IsMariko();
    if (mariko) { for (u32 i = 0; i < 6; ++i) CalcCal(mS[i],bcp,bft,tcp,tft,cal[i]); }
    else        { for (u32 i = 0; i < 8; ++i) CalcCal(eS[i],bcp,bft,tcp,tft,cal[i]); }

    StartSensors();
    ready = true;
}

// Read current die temperatures into out_cpu/out_gpu/out_mem (milliCelsius).
// Call once per refresh cycle.  Handles sleep/wake by re-enabling sensors when
// TSENSOR_CLKEN is cleared by HW.
static void Read(u32& out_cpu, u32& out_gpu, u32& out_mem) {
    if (!ready || !vSoc) return;
    // Sleep detection: HW clears TSENSOR_CLKEN on suspend; re-arm on wake.
    if (Rd(vSoc, TSENSOR_CLKEN) != TSENSOR_ENABLE) { StartSensors(); return; }
    u32 t1 = Rd(vSoc, SENSOR_TEMP1);
    u32 t2 = Rd(vSoc, SENSOR_TEMP2);
    if (!t1 && !t2) return; // sensors still warming up — keep last values
    out_cpu = (u32)Trans((u16)(t1 >> 16));
    out_gpu = (u32)Trans((u16)(t1 & 0xFFFFu));
    // Erista: dedicated MEM sensor in TEMP2[31:16]
    // Mariko: no dedicated MEM sensor — PLLX [15:0] used as proxy (matches HOC)
    out_mem = IsMariko()
        ? (u32)Trans((u16)(t2 & 0xFFFFu))
        : (u32)Trans((u16)(t2 >> 16));
}

} // namespace Soctherm
