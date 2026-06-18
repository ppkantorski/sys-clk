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
#include "../../soctherm.h"

// Cache hardware model to avoid repeated syscalls
static bool g_hardwareModelCached = false;
bool g_isMariko = false;

bool IsMariko() {
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

bool IsErista() {
    return !IsMariko();
}

// ── Config-INI helpers (overlay-only key) ──────────────────────────────────
// Stored in the [overlay] section of /config/sys-clk/config.ini.
// File-scope statics so they don't pollute the class API.

static constexpr const char* CONFIG_PATH     = "/config/sys-clk/config.ini";
static constexpr const char* OVERLAY_SECTION = "[overlay]";
static constexpr const char* COMP_TEMPS_KEY  = "show_component_temps";

static bool readOverlayBool(const char* key, bool defaultValue = false) {
    FILE* file = fopen(CONFIG_PATH, "r");
    if (!file) return defaultValue;

    char line[256];
    bool inOverlay = false;
    bool result    = defaultValue;

    while (fgets(line, sizeof(line), file)) {
        // Strip trailing CR/LF
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        if (strcmp(line, OVERLAY_SECTION) == 0) { inOverlay = true; continue; }
        if (inOverlay && line[0] == '[')         { break; }  // next section
        if (!inOverlay)                          { continue; }

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        if (strcmp(line, key) == 0) {
            result = (atoi(eq + 1) != 0);
            break;
        }
    }
    fclose(file);
    return result;
}

static int readOverlayInt(const char* key, int defaultValue) {
    FILE* file = fopen(CONFIG_PATH, "r");
    if (!file) return defaultValue;

    char line[256];
    bool inOverlay = false;
    int  result    = defaultValue;

    while (fgets(line, sizeof(line), file)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        if (strcmp(line, OVERLAY_SECTION) == 0) { inOverlay = true; continue; }
        if (inOverlay && line[0] == '[')         { break; }
        if (!inOverlay)                          { continue; }

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        if (strcmp(line, key) == 0) {
            result = atoi(eq + 1);
            break;
        }
    }
    fclose(file);
    return result;
}

static void writeOverlayInt(const char* key, int value) {
    FILE* file = fopen(CONFIG_PATH, "r");
    if (!file) return;

    std::vector<std::string> lines;
    char buf[256];
    int  overlaySectionIndex = -1;
    int  existingKeyIndex    = -1;
    bool inOverlay           = false;

    while (fgets(buf, sizeof(buf), file)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
        lines.push_back(buf);

        int idx = (int)lines.size() - 1;
        if (strcmp(buf, OVERLAY_SECTION) == 0) { inOverlay = true; overlaySectionIndex = idx; continue; }
        if (inOverlay && buf[0] == '[')        { inOverlay = false; continue; }
        if (!inOverlay)                        { continue; }

        char tmp[256];
        strncpy(tmp, buf, sizeof(tmp));
        char* eq = strchr(tmp, '=');
        if (!eq) continue;
        *eq = '\0';
        if (strcmp(tmp, key) == 0) existingKeyIndex = idx;
    }
    fclose(file);

    char valBuf[32];
    snprintf(valBuf, sizeof(valBuf), "%d", value);
    std::string entry = std::string(key) + "=" + valBuf;

    if (existingKeyIndex >= 0) {
        lines[existingKeyIndex] = entry;
    } else if (overlaySectionIndex >= 0) {
        lines.insert(lines.begin() + overlaySectionIndex + 1, entry);
    } else {
        lines.push_back("");
        lines.push_back(OVERLAY_SECTION);
        lines.push_back(entry);
    }

    FILE* out = fopen(CONFIG_PATH, "w");
    if (!out) return;
    for (const auto& l : lines) fprintf(out, "%s\n", l.c_str());
    fclose(out);
}

static void writeOverlayBool(const char* key, bool value) {
    FILE* file = fopen(CONFIG_PATH, "r");
    if (!file) return;

    std::vector<std::string> lines;
    char buf[256];
    int  overlaySectionIndex = -1;
    int  existingKeyIndex    = -1;
    bool inOverlay           = false;

    while (fgets(buf, sizeof(buf), file)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
        lines.push_back(buf);

        int idx = (int)lines.size() - 1;
        if (strcmp(buf, OVERLAY_SECTION) == 0) { inOverlay = true; overlaySectionIndex = idx; continue; }
        if (inOverlay && buf[0] == '[')        { inOverlay = false; continue; }
        if (!inOverlay)                        { continue; }

        char tmp[256];
        strncpy(tmp, buf, sizeof(tmp));
        char* eq = strchr(tmp, '=');
        if (!eq) continue;
        *eq = '\0';
        if (strcmp(tmp, key) == 0) existingKeyIndex = idx;
    }
    fclose(file);

    std::string entry = std::string(key) + "=" + (value ? "1" : "0");

    if (existingKeyIndex >= 0) {
        // Key already exists — update it in place
        lines[existingKeyIndex] = entry;
    } else if (overlaySectionIndex >= 0) {
        // Section exists but key is new — insert after section header
        lines.insert(lines.begin() + overlaySectionIndex + 1, entry);
    } else {
        // No [overlay] section yet — append it at the end of the file
        lines.push_back("");
        lines.push_back(OVERLAY_SECTION);
        lines.push_back(entry);
    }

    FILE* out = fopen(CONFIG_PATH, "w");
    if (!out) return;
    for (const auto& l : lines) fprintf(out, "%s\n", l.c_str());
    fclose(out);
}
// ──────────────────────────────────────────────────────────────────────────

// ── Overlay refresh rate ──────────────────────────────────────────────────
static constexpr const char* REFRESH_RATE_KEY = "refresh_rate_hz";

// Nanosecond interval derived from the Hz setting.  Loaded once at startup
// and updated immediately when the user changes the dropdown.
static u64 g_refreshIntervalNs = 1000000000UL; // default 1 Hz

// Called from RefreshRateGui after writing the new value to the INI.
void BaseMenuGui::applyRefreshRateHz(int hz) {
    if (hz <= 0) hz = 1;
    g_refreshIntervalNs = 1000000000UL / (u64)hz;
    writeOverlayInt(REFRESH_RATE_KEY, hz);
}

int BaseMenuGui::getRefreshRateHz() {
    return readOverlayInt(REFRESH_RATE_KEY, 1);
}
// ──────────────────────────────────────────────────────────────────────────

BaseMenuGui::BaseMenuGui()
{
    isUsingHOC = usingHOC();
    isUsingEOS = usingEOS(); // Must be set here — createUI() runs later
    //tsl::initializeThemeVars();
    this->context = nullptr;
    this->lastContextUpdate = 0;
    this->listElement = nullptr;
    
    // Initialize all voltages to zero once
    memset(&cpuVoltageUv, 0, sizeof(u32) * 5); // Zero all 5 voltage values at once
    
    // Initialize component temps to zero
    componentCPU_mC = 0;
    componentGPU_mC = 0;
    componentRAM_mC = 0;
    
    m_touchStartedInRect = false;
    
    // Pre-cache hardware model during initialization
    IsMariko();
    
    // Initialize display strings
    memset(displayStrings, 0, sizeof(displayStrings));

    // Restore persisted freq/temp toggle state from config.ini (once per session).
    // Subsequent BaseMenuGui instances (e.g. navigating away and back) reuse the
    // already-loaded static value and don't re-read the file.
    static bool s_tempStateLoaded = false;
    if (!s_tempStateLoaded) {
        s_tempStateLoaded = true;
        m_showComponentTemps = readOverlayBool(COMP_TEMPS_KEY, true);
        // Load and apply the persisted refresh rate
        int hz = readOverlayInt(REFRESH_RATE_KEY, 1);
        if (hz <= 0) hz = 1;
        g_refreshIntervalNs = 1000000000UL / (u64)hz;
    }

    // HOC reads component temps via IPC; EOS and stock use SOCTHERM hardware directly.
    if (!isUsingHOC) {
        Soctherm::Initialize();
    }
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
    renderer->drawRoundedRect(14, 70-1, 420, 30+2, 12.0f, renderer->aWithOpacity(tsl::tableBGColor));
    const auto w2 = tsl::makeSwitch2Wheel(
        0xFF57,   // anchor[0] UR — fixed peak: Muted Violet-Steel  (r=7, g=5, b=F, a=F)
        0xFF46,   // anchor[2] LL — fixed peak: Deep Slate           (r=6, g=4, b=F, a=F)
        0xF997,   // anchor[1] LR — hero bright: dim Warm Steel      (r=7, g=9, b=9, a=F)
        0xF756,   // anchor[1] LR — hero deep:   dark Slate Navy     (r=6, g=5, b=7, a=F)
        0xF89A,   // anchor[3] UL — hero bright: dim Periwinkle      (r=A, g=9, b=8, a=F)
        0xF557,   // anchor[3] UL — hero deep:   dark Indigo Gray    (r=7, g=5, b=5, a=F)
        12.0,
        true
    );
    renderer->drawBorderedRoundedRect(14+2, 70-1, 420, 30+2, 1.0, 12.0f, renderer->aWithOpacity(tsl::widgetBorderColor), &w2);
    
    // App ID - use pre-formatted string
    renderer->drawString(labels[0], false, positions[0], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(displayStrings[0], false, positions[0] + labelWidths[0] + 9, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
    
    // Profile - use pre-formatted string
    // Label is fixed; value is centered within its reserved slot [423 - maxProfileValueWidth, 423]
    renderer->drawString(labels[1], false, 423 - maxProfileValueWidth - labelWidths[1] - 9, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    {
        u32 profileValueWidth = renderer->getTextDimensions(displayStrings[1], false, SMALL_TEXT_SIZE).first;
        u32 profileValueX = (423 - maxProfileValueWidth) + (maxProfileValueWidth - profileValueWidth) / 2;
        renderer->drawString(displayStrings[1], false, profileValueX, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
    }
    
    y = 129; // Direct assignment instead of += 38
    
    // === MAIN DATA SECTION ===
    renderer->drawRoundedRect(14, 106, 420, 116, 12.0f, renderer->aWithOpacity(tsl::tableBGColor));
    renderer->drawBorderedRoundedRect(14+2, 106, 420, 116, 1.0, 12.0f, renderer->aWithOpacity(tsl::widgetBorderColor), &w2);
    
    // === FREQUENCY SECTION ===
    // Labels first (better cache locality)
    renderer->drawString(labels[2], false, positions[2], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[3], false, positions[3], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    renderer->drawString(labels[4], false, positions[4], y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
    
    // Top freq row:
    //   HOC  mode: showTemps=true → HOC IPC component temps; false → target freqs (default)
    //   non-HOC:   showTemps=true → SOCTHERM die temps (default); false → target freqs
    // m_showComponentTemps carries the same meaning in both cases.
    const bool showTemps = m_showComponentTemps;
    renderer->drawString(showTemps ? displayStrings[17] : displayStrings[2], false, dataPositions[0], y, SMALL_TEXT_SIZE, showTemps ? tempColors[3] : tsl::infoTextColor);  // CPU
    renderer->drawString(showTemps ? displayStrings[18] : displayStrings[3], false, dataPositions[1], y, SMALL_TEXT_SIZE, showTemps ? tempColors[4] : tsl::infoTextColor);  // GPU
    renderer->drawString(showTemps ? displayStrings[19] : displayStrings[4], false, dataPositions[2], y, SMALL_TEXT_SIZE, showTemps ? tempColors[5] : tsl::infoTextColor);  // MEM
    
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

Result sysclkCheck = 1;

// Persist the freq/temp toggle across GUI re-creation (navigation away and back).
bool BaseMenuGui::m_showComponentTemps = false;


// Optimized refresh - now does all the string formatting once per second
void BaseMenuGui::refresh()
{
    const u64 ticks = armGetSystemTick();
    // Use cached comparison based on configured refresh rate
    if (armTicksToNs(ticks - this->lastContextUpdate) <= g_refreshIntervalNs) [[likely]] {
        return; // Early exit for most calls
    }
    
    this->lastContextUpdate = ticks;
    
    // Lazy context allocation
    if (!this->context) [[unlikely]] {
        this->context = new SysClkContext;
    }

    
    if (R_SUCCEEDED(sysclkIpcGetCurrentContext(this->context))) {
        // realVolts is a HOC-exclusive IPC extension; EOS does not populate it.
        if (isUsingHOC) {
            cpuVoltageUv = this->context->realVolts[0]; 
            gpuVoltageUv = this->context->realVolts[1]; 
            socVoltageUv = this->context->realVolts[3];
            
            // Unpack realVolts[2] into separate EMC and VDD voltages
            const u32 packed = this->context->realVolts[2];
            const float vdd2_mV_f = packed / 100000.0f;
            const u32 vddq_mV = (packed % 10000) / 10;
            
            vddVoltageUv = (u32)(vdd2_mV_f * 1000);
            emcVoltageUv = vddq_mV * 1000;
        }
    }

    // EOS and stock both read voltages from hardware regulator directly.
    if (!isUsingHOC) {
        // === ULTRA-FAST VOLTAGE READING ===
        // Pre-computed domain configuration based on hardware
        static constexpr PowerDomainId domains[] = {
            PcvPowerDomainId_Max77621_Cpu,    // [0] CPU
            PcvPowerDomainId_Max77621_Gpu,    // [1] GPU  
            PcvPowerDomainId_Max77812_Dram,   // [2] EMC/DRAM - Mariko only
            PcvPowerDomainId_Max77620_Sd0,    // [3] SOC - HOC only
            PcvPowerDomainId_Max77620_Sd1     // [4] VDD2 - HOC only
        };
        
        // Voltage array for direct indexing
        u32* voltages[] = {&cpuVoltageUv, &gpuVoltageUv, &emcVoltageUv, &socVoltageUv, &vddVoltageUv};

        // Helper to safely read a voltage or set it to 0
        auto readVoltage = [&](int idx) {
            RgltrSession session;
            if (R_SUCCEEDED(rgltrOpenSession(&session, domains[idx]))) {
                if (R_FAILED(rgltrGetVoltage(&session, voltages[idx]))) {
                    *voltages[idx] = 0;
                }
                rgltrCloseSession(&session);
            } else {
                *voltages[idx] = 0;
            }
        };
        
        // Single regulator init/exit cycle
        if (R_SUCCEEDED(rgltrInitialize())) [[likely]] {
            for (int i = 0; i < 5; ++i) {
                if (!IsMariko() && i == 2) {
                    *voltages[i] = 0; // Skip DRAM domain for Erista
                    continue;
                }
                readVoltage(i);
            }
        
            if (!IsMariko()) {
                emcVoltageUv = 0; // Erista never supports DRAM
            }
        
            rgltrExit();
        } else {
            // Zero all voltages on regulator failure
            memset(&cpuVoltageUv, 0, sizeof(u32) * 5);
        }
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

    // HOC reads per-component die temps via IPC; EOS and stock use SOCTHERM.
    if (isUsingHOC) {
        u32 ct = context->componentTemps[0]; // CPU die
        sprintf(displayStrings[17], "%u.%u °C", ct / 1000U, (ct % 1000U) / 100U);
        tempColors[3] = tsl::GradientColor(ct * 0.001f, tsl::DEFAULT_TEMP_RANGE_HIGH);

        ct = context->componentTemps[1]; // GPU die
        sprintf(displayStrings[18], "%u.%u °C", ct / 1000U, (ct % 1000U) / 100U);
        tempColors[4] = tsl::GradientColor(ct * 0.001f, tsl::DEFAULT_TEMP_RANGE_HIGH);

        ct = context->componentTemps[2]; // MEM / PLLX
        sprintf(displayStrings[19], "%u.%u °C", ct / 1000U, (ct % 1000U) / 100U);
        tempColors[5] = tsl::GradientColor(ct * 0.001f, tsl::DEFAULT_TEMP_RANGE_HIGH);
    } else {
        // Stock and EOS: read CPU/GPU/MEM die temps directly from SOCTHERM hardware.
        Soctherm::Initialize();
        Soctherm::Read(componentCPU_mC, componentGPU_mC, componentRAM_mC);

        u32 ct = componentCPU_mC;
        sprintf(displayStrings[17], "%u.%u °C", ct / 1000U, (ct % 1000U) / 100U);
        tempColors[3] = tsl::GradientColor(ct * 0.001f, tsl::DEFAULT_TEMP_RANGE_HIGH);

        ct = componentGPU_mC;
        sprintf(displayStrings[18], "%u.%u °C", ct / 1000U, (ct % 1000U) / 100U);
        tempColors[4] = tsl::GradientColor(ct * 0.001f, tsl::DEFAULT_TEMP_RANGE_HIGH);

        ct = componentRAM_mC;
        sprintf(displayStrings[19], "%u.%u °C", ct / 1000U, (ct % 1000U) / 100U);
        tempColors[5] = tsl::GradientColor(ct * 0.001f, tsl::DEFAULT_TEMP_RANGE_HIGH);
    }
}

bool BaseMenuGui::handleInput(u64 keysDown, u64 keysHeld,
                              const HidTouchState& touchPos,
                              HidAnalogStickState leftJoyStick,
                              HidAnalogStickState rightJoyStick)
{
    // + button toggles between target-freq display and per-component die temp
    // display in the CPU/GPU/MEM row.
    //   HOC  mode: default = target freqs → toggle + to show HOC IPC component temps
    //   non-HOC:   default = SOCTHERM die temps → toggle + to show target freqs
    if (keysDown & KEY_PLUS) {
        m_showComponentTemps = !m_showComponentTemps;
        writeOverlayBool(COMP_TEMPS_KEY, m_showComponentTemps);
        triggerMoveFeedback();
        return true; // consumed — don't pass to list
    }

    // Touch-tap on the main data table does the same toggle on release.
    // The table rect is drawRoundedRect(14, 106, 420, 116) → x:[14,434] y:[106,222].
    static constexpr u32 RECT_X = 14, RECT_Y = 106, RECT_W = 420, RECT_H = 116;
    const bool touching = ult::stillTouching.load(std::memory_order_acquire);
    const bool inRect   = touchPos.x >= RECT_X && touchPos.x <= RECT_X + RECT_W &&
                          touchPos.y >= RECT_Y && touchPos.y <= RECT_Y + RECT_H;

    if (touching && inRect && !m_touchStartedInRect) {
        // First frame of a touch that began inside the rect.
        m_touchStartedInRect = true;
        triggerNavigationFeedback();
    } else if (!touching && m_touchStartedInRect) {
        // Finger lifted after starting inside the rect — trigger toggle.
        m_showComponentTemps = !m_showComponentTemps;
        writeOverlayBool(COMP_TEMPS_KEY, m_showComponentTemps);
        triggerMoveFeedback();
        m_touchStartedInRect = false;
    } else if (!touching) {
        // No touch active — clear flag (handles the case where touch
        // started outside rect and we were never tracking it).
        m_touchStartedInRect = false;
    }

    return false; // let the list handle everything else
}

tsl::elm::Element* BaseMenuGui::baseUI()
{
    auto* list = new tsl::elm::List();
    this->listElement = list;
    this->listElement->setCenterOffset(-61.0f - 2.0f);
    this->listUI();

    return list;
}