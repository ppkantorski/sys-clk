/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "base_menu_gui.h"
#include "fatal_gui.h"

// Cache hardware model to avoid repeated syscalls
static bool g_hardwareModelCached = false;
static bool g_isMariko = false;

static inline bool IsMariko() {
    if (!g_hardwareModelCached) {
        SetSysProductModel model = SetSysProductModel_Invalid;
        setsysGetProductModel(&model);
        g_isMariko = (model == SetSysProductModel_Iowa || 
                     model == SetSysProductModel_Hoag || 
                     model == SetSysProductModel_Calcio || 
                     model == SetSysProductModel_Aula);
        g_hardwareModelCached = true;
    }
    return g_isMariko;
}

static inline bool IsErista() {
    return !IsMariko();
}

BaseMenuGui::BaseMenuGui() : tempColors{tsl::Color(0), tsl::Color(0), tsl::Color(0)}
{
    isUsingEOS = usingEOS();
    tsl::initializeThemeVars();
    this->context = nullptr;
    this->lastContextUpdate = 0;
    this->listElement = nullptr;
    
    // Initialize all voltages to zero once
    memset(&cpuVoltageUv, 0, sizeof(u32) * 5); // Zero all 5 voltage values at once
    
    // Pre-cache hardware model during initialization
    IsMariko();
    
    // Initialize display strings
    memset(displayStrings, 0, sizeof(displayStrings));
}

BaseMenuGui::~BaseMenuGui() {
    delete this->context; // delete handles nullptr automatically
}

// Fast preDraw - just renders pre-computed strings
void BaseMenuGui::preDraw(tsl::gfx::Renderer* renderer) {
    BaseGui::preDraw(renderer);
    if(!this->context) [[unlikely]] return;
    
    // All constants pre-calculated and cached
    static constexpr const char* const labels[10] = {
        "App ID", "Profile", "CPU", "GPU", "MEM", "SOC", "PCB", "Skin", "Now", "Avg"
    };

    static constexpr u32 dataPositions[6] = {63-3+3, 200-1, 344-1-3, 200-1, 342-1, 321-1};
    
    static u32 labelWidths[10];
    static bool positionsInitialized = false;

    if (!positionsInitialized) {
        for (int i = 0; i < 10; i++) {
            labelWidths[i] = renderer->getTextDimensions(labels[i], false, SMALL_TEXT_SIZE).first;
        }
        positionsInitialized = true;
    }
    static u32 positions[10] = {24-1, 310-labelWidths[1], 24-1, 192-labelWidths[3], 332-labelWidths[4], 24-1, 192 - labelWidths[6], 332-labelWidths[7], 192 - labelWidths[8], 332-labelWidths[9]};

    static u32 maxProfileValueWidth = renderer->getTextDimensions("Official Charger", false, SMALL_TEXT_SIZE).first; // longest word

    u32 y = 91;
    
    // === TOP SECTION ===
    renderer->drawRoundedRect(14, 70-1, 420, 30+2, 10.0f, renderer->aWithOpacity(tsl::tableBGColor));
    
    // App ID - use pre-formatted string
    renderer->drawString(labels[0], false, positions[0], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(displayStrings[0], false, positions[0] + labelWidths[0] + 9, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
    
    // Profile - use pre-formatted string
    renderer->drawString(labels[1], false, 423 - maxProfileValueWidth - labelWidths[1] - 9, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(displayStrings[1], false, 423 - maxProfileValueWidth, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
    
    y = 129; // Direct assignment instead of += 38
    
    // === MAIN DATA SECTION ===
    renderer->drawRoundedRect(14, 106, 420, 116, 10.0f, renderer->aWithOpacity(tsl::tableBGColor));
    
    // === FREQUENCY SECTION ===
    // Labels first (better cache locality)
    renderer->drawString(labels[2], false, positions[2], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[3], false, positions[3], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[4], false, positions[4], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    
    // Current frequencies - use pre-formatted strings
    renderer->drawString(displayStrings[2], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // CPU
    renderer->drawString(displayStrings[3], false, dataPositions[1], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // GPU
    renderer->drawString(displayStrings[4], false, dataPositions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // MEM
    
    y = 149; // Direct assignment (129 + 20)
    
    // === REAL FREQUENCIES ===
    renderer->drawString(displayStrings[5], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // CPU real
    renderer->drawString(displayStrings[6], false, dataPositions[1], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // GPU real
    renderer->drawString(displayStrings[7], false, dataPositions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // MEM real
    
    y = 169; // Direct assignment (149 + 20)
    
    // === VOLTAGES ===
    renderer->drawString(displayStrings[8], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // CPU voltage
    renderer->drawString(displayStrings[9], false, dataPositions[1], y, SMALL_TEXT_SIZE, tsl::infoTextColor);   // GPU voltage
    
    // Memory voltage - check if VDD is present
    if (emcVoltageUv && vddVoltageUv) {
        renderer->drawStringWithColoredSections(displayStrings[10], false, {""}, dataPositions[5]-16, y, SMALL_TEXT_SIZE, tsl::infoTextColor, tsl::separatorColor);
    } else if (vddVoltageUv) {
        renderer->drawString(displayStrings[10], false, dataPositions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor);
    } else if (emcVoltageUv) {
        renderer->drawString(displayStrings[10], false, dataPositions[2], y, SMALL_TEXT_SIZE, tsl::infoTextColor);
    }
    
    y = 191; // Direct assignment (169 + 22)
    
    // === TEMPERATURE SECTION ===
    // Labels
    renderer->drawString(labels[5], false, positions[5], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[6], false, positions[6]-1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[7], false, positions[7], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    
    // Temperatures with color - use pre-computed colors
    renderer->drawString(displayStrings[11], false, dataPositions[0], y, SMALL_TEXT_SIZE, tempColors[0]);  // SOC
    renderer->drawString(displayStrings[12], false, dataPositions[1], y, SMALL_TEXT_SIZE, tempColors[1]);  // PCB
    renderer->drawString(displayStrings[13], false, dataPositions[2], y, SMALL_TEXT_SIZE, tempColors[2]);  // Skin
    
    y = 211; // Direct assignment (191 + 20)
    
    // === SOC VOLTAGE & POWER ===
    // SOC voltage (if available)
    if (socVoltageUv) [[likely]] {
        renderer->drawString(displayStrings[14], false, dataPositions[0], y, SMALL_TEXT_SIZE, tsl::infoTextColor);
    }
    
    // Power labels and values
    renderer->drawString(labels[8], false, positions[8]-1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[9], false, positions[9], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    
    renderer->drawString(displayStrings[15], false, dataPositions[3], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // Power now
    renderer->drawString(displayStrings[16], false, dataPositions[4], y, SMALL_TEXT_SIZE, tsl::infoTextColor);  // Power avg
}

// Optimized refresh - now does all the string formatting once per second
void BaseMenuGui::refresh()
{
    const u64 ticks = armGetSystemTick();
    // Use cached comparison - 1 billion nanoseconds
    if (armTicksToNs(ticks - this->lastContextUpdate) <= 1000000000UL) [[likely]] {
        return; // Early exit for most calls
    }
    
    this->lastContextUpdate = ticks;
    
    // Lazy context allocation
    if (!this->context) [[unlikely]] {
        this->context = new SysClkContext;
    }

    // === ULTRA-FAST VOLTAGE READING ===
    // Pre-computed domain configuration based on hardware
    static const PowerDomainId domains[] = {
        PcvPowerDomainId_Max77621_Cpu,    // [0] CPU
        PcvPowerDomainId_Max77621_Gpu,    // [1] GPU  
        PcvPowerDomainId_Max77812_Dram,   // [2] EMC/DRAM - Mariko only
        PcvPowerDomainId_Max77620_Sd0,    // [3] SOC - EOS only
        PcvPowerDomainId_Max77620_Sd1     // [4] VDD2 - EOS only
    };
    
    // Voltage array for direct indexing
    u32* voltages[] = {&cpuVoltageUv, &gpuVoltageUv, &emcVoltageUv, &socVoltageUv, &vddVoltageUv};
    
    // Single regulator init/exit cycle
    if (R_SUCCEEDED(rgltrInitialize())) [[likely]] {
        if (IsMariko()) {
            if (isUsingEOS) {
                // Mariko with EOS: all 5 domains
                for (int i = 0; i < 5; ++i) {
                    RgltrSession session;
                    if (R_SUCCEEDED(rgltrOpenSession(&session, domains[i]))) [[likely]] {
                        if (R_FAILED(rgltrGetVoltage(&session, voltages[i]))) {
                            *voltages[i] = 0;
                        }
                        rgltrCloseSession(&session);
                    } else {
                        *voltages[i] = 0;
                    }
                }
            } else {
                // Mariko without EOS: CPU, GPU, DRAM only (no Sd0/Sd1)
                for (int i = 0; i < 3; ++i) {
                    RgltrSession session;
                    if (R_SUCCEEDED(rgltrOpenSession(&session, domains[i]))) [[likely]] {
                        if (R_FAILED(rgltrGetVoltage(&session, voltages[i]))) {
                            *voltages[i] = 0;
                        }
                        rgltrCloseSession(&session);
                    } else {
                        *voltages[i] = 0;
                    }
                }
                socVoltageUv = vddVoltageUv = 0;
            }
        } else {
            // Erista
            if (isUsingEOS) {
                // Erista with EOS: CPU, GPU, SOC, VDD (no DRAM)
                for (int i = 0; i < 5; ++i) {
                    if (i == 2) continue; // Skip DRAM domain
                    
                    RgltrSession session;
                    if (R_SUCCEEDED(rgltrOpenSession(&session, domains[i]))) [[likely]] {
                        if (R_FAILED(rgltrGetVoltage(&session, voltages[i]))) {
                            *voltages[i] = 0;
                        }
                        rgltrCloseSession(&session);
                    } else {
                        *voltages[i] = 0;
                    }
                }
                emcVoltageUv = 0; // Erista never supports DRAM
            } else {
                // Erista without EOS: CPU and GPU only
                for (int i = 0; i < 2; ++i) {
                    RgltrSession session;
                    if (R_SUCCEEDED(rgltrOpenSession(&session, domains[i]))) [[likely]] {
                        if (R_FAILED(rgltrGetVoltage(&session, voltages[i]))) {
                            *voltages[i] = 0;
                        }
                        rgltrCloseSession(&session);
                    } else {
                        *voltages[i] = 0;
                    }
                }
                emcVoltageUv = socVoltageUv = vddVoltageUv = 0;
            }
        }
        
        rgltrExit();
    } else {
        // Zero all voltages on regulator failure
        memset(&cpuVoltageUv, 0, sizeof(u32) * 5);
    }

    // === SYSCLK CONTEXT UPDATE ===
    const Result rc = sysclkIpcGetCurrentContext(this->context);
    if (R_FAILED(rc)) [[unlikely]] {
        FatalGui::openWithResultCode("sysclkIpcGetCurrentContext", rc);
        return;
    }
    
    // === FORMAT ALL DISPLAY STRINGS (once per second) ===
    // App ID (hex conversion)
    sprintf(displayStrings[0], "%016lX", context->applicationId);
    
    // Profile
    strcpy(displayStrings[1], sysclkFormatProfile(context->profile, true));
    
    // Current frequencies
    u32 hz = context->freqs[0]; // CPU
    sprintf(displayStrings[2], "%u.%u MHz", hz / 1000000U, (hz / 100000U) % 10U);
    
    hz = context->freqs[1]; // GPU
    sprintf(displayStrings[3], "%u.%u MHz", hz / 1000000U, (hz / 100000U) % 10U);
    
    hz = context->freqs[2]; // MEM
    sprintf(displayStrings[4], "%u.%u MHz", hz / 1000000U, (hz / 100000U) % 10U);
    
    // Real frequencies
    hz = context->realFreqs[0]; // CPU
    sprintf(displayStrings[5], "%u.%u MHz", hz / 1000000U, (hz / 100000U) % 10U);
    
    hz = context->realFreqs[1]; // GPU
    sprintf(displayStrings[6], "%u.%u MHz", hz / 1000000U, (hz / 100000U) % 10U);
    
    hz = context->realFreqs[2]; // MEM
    sprintf(displayStrings[7], "%u.%u MHz", hz / 1000000U, (hz / 100000U) % 10U);
    
    // Voltages
    sprintf(displayStrings[8], "%u mV", cpuVoltageUv / 1000U);
    sprintf(displayStrings[9], "%u mV", gpuVoltageUv / 1000U);
    
    // Memory voltage (handle VDD case)
    if (emcVoltageUv && vddVoltageUv) {
        //sprintf(displayStrings[10], "%u%u mV", vddVoltageUv / 1000U, emcVoltageUv / 1000U);
        //sprintf(displayStrings[10], "%u%.1f mV", vddVoltageUv / 1000U, emcVoltageUv / 1000.0f);
        sprintf(displayStrings[10], "%u.%u%u mV", vddVoltageUv / 1000U, (vddVoltageUv % 1000U) / 100U, emcVoltageUv / 1000U);
    } else if (vddVoltageUv) {
        //sprintf(displayStrings[10], "%u mV", vddVoltageUv / 1000U);
        sprintf(displayStrings[10], "%u.%u mV", vddVoltageUv / 1000U, (vddVoltageUv % 1000U) / 100U);
    } else if (emcVoltageUv) {
        sprintf(displayStrings[10], "%u mV", emcVoltageUv / 1000U);
    }
    
    // Temperatures and pre-compute colors
    u32 millis = context->temps[0]; // SOC
    sprintf(displayStrings[11], "%u.%u °C", millis / 1000U, (millis % 1000U) / 100U);
    tempColors[0] = tsl::GradientColor(millis * 0.001f);
    
    millis = context->temps[1]; // PCB
    sprintf(displayStrings[12], "%u.%u °C", millis / 1000U, (millis % 1000U) / 100U);
    tempColors[1] = tsl::GradientColor(millis * 0.001f);
    
    millis = context->temps[2]; // Skin
    sprintf(displayStrings[13], "%u.%u °C", millis / 1000U, (millis % 1000U) / 100U);
    tempColors[2] = tsl::GradientColor(millis * 0.001f);
    
    // SOC voltage (if available)
    if (socVoltageUv) {
        sprintf(displayStrings[14], "%u mV", socVoltageUv / 1000U);
    }
    
    // Power
    sprintf(displayStrings[15], "%d mW", context->power[0]); // Now
    sprintf(displayStrings[16], "%d mW", context->power[1]); // Avg
}

tsl::elm::Element* BaseMenuGui::baseUI()
{
    auto* list = new tsl::elm::List();
    this->listElement = list;
    this->listUI();

    return list;
}