#include "misc_gui.h"
#include "fatal_gui.h"
#include "../format.h"
#include <cstdio>
#include <cstring>
//#include <sstream>
#include <climits>

// ── Governor minimum-frequency options (HOC) ────────────────────────────────
// Real Switch DVFS steps ONLY — the governor floors newHz to one of these and
// then snaps through the hardware table, so off-table values would stall it.
//   CPU: 510 → 1581 MHz (102 MHz steps, then the 1428→1581 hardware jump).
//   GPU: 153.6 → 844.8 MHz (76.8 MHz steps).  Default & lowest = 153.6 MHz.
// The label macros are passed straight into NamedStepTrackBar (a braced-init
// list binds whether its param is std::initializer_list or std::vector); the
// parallel Hz arrays drive the index<->Hz mapping so the two never drift.
namespace {
    constexpr int kCpuGovMinHz[] = {
        510000000, 612000000, 714000000, 816000000, 918000000, 1020000000,
        1122000000, 1224000000, 1326000000, 1428000000, 1581000000
    };
    constexpr int kGpuGovMinHz[] = {
        76800000, 153600000, 230400000, 307200000, 384000000, 460800000,
        537600000, 614400000, 691200000, 768000000, 844800000
    };
    constexpr int kCpuGovMinCount = (int)(sizeof(kCpuGovMinHz) / sizeof(kCpuGovMinHz[0]));
    constexpr int kGpuGovMinCount = (int)(sizeof(kGpuGovMinHz) / sizeof(kGpuGovMinHz[0]));

    // Nearest option index for a stored Hz value. Tolerates legacy / off-step
    // values (e.g. an old cpu_gov_min_freq that isn't in the new list).
    inline int govMinNearestIndex(const int* arr, int count, int hz) {
        int best = 0; long bestDiff = LONG_MAX;
        for (int i = 0; i < count; i++) {
            long d = (long)arr[i] - (long)hz; if (d < 0) d = -d;
            if (d < bestDiff) { bestDiff = d; best = i; }
        }
        return best;
    }
}
#define CPU_GOV_MIN_LABELS { "510 MHz","612 MHz","714 MHz","816 MHz","918 MHz","1020 MHz","1122 MHz","1224 MHz","1326 MHz","1428 MHz","1581 MHz" }
#define GPU_GOV_MIN_LABELS { "76 MHz","153 MHz","230 MHz","307 MHz","384 MHz","460 MHz","537 MHz","614 MHz","691 MHz","768 MHz","844 MHz" }

// ── MiscGui ───────────────────────────────────────────────────────────────

MiscGui::MiscGui()
{
    // Load current config values — keys differ between EOS and HOC sysmodules
    configValues["uncapped_clocks"] = getConfigValue("uncapped_clocks");
    configValues["reversenx_sync"]  = getConfigValue("reversenx_sync");

    if (usingEOS()) {
        configValues["auto_cpu_boost"]  = getConfigValue("auto_cpu_boost");
        // EOS sysmodule uses boost_gpu_override instead of ow_boost, and has no
        // allow_governing or dvfs_mode.  The int keys (auto_gpu_vmin,
        // gpu_vmin_offset) are handled separately as trackbars.
        configValues["boost_gpu_override"] = getConfigValue("boost_gpu_override");
    } else {
        // HOC sysmodule keys
        configValues["ow_boost"]        = getConfigValue("ow_boost");
        configValues["allow_governing"] = getConfigValue("allow_governing", false);
        configValues["dvfs_mode"]       = getConfigValue("dvfs_mode", true);
        // dvfs_offset handled separately as a trackbar
    }
}

MiscGui::~MiscGui()
{
    this->configToggles.clear();
}

bool MiscGui::getConfigValue(const std::string& iniKey, bool defaultValue)
{
    FILE* file = fopen("/config/sys-clk/config.ini", "r");
    if (!file) {
        return defaultValue;
    }
    
    char line[512];
    bool inValuesSection = false;

    size_t len;
    while (fgets(line, sizeof(line), file)) {
        // Remove newline if present
        len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Trim whitespace
        char* start = line;
        while (*start == ' ' || *start == '\t') start++;
        char* end = start + strlen(start) - 1;
        while (end > start && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        // Check for [values] section
        if (strcmp(start, "[values]") == 0) {
            inValuesSection = true;
            continue;
        }
        
        // Check for new section
        if (strlen(start) > 0 && start[0] == '[') {
            inValuesSection = false;
            continue;
        }
        
        // Parse key=value in values section
        if (inValuesSection) {
            char* equalPos = strchr(start, '=');
            if (equalPos != nullptr) {
                *equalPos = '\0'; // Split the string
                char* key = start;
                char* value = equalPos + 1;
                
                // Trim key
                char* keyEnd = key + strlen(key) - 1;
                while (keyEnd > key && (*keyEnd == ' ' || *keyEnd == '\t')) {
                    *keyEnd = '\0';
                    keyEnd--;
                }
                
                // Trim value
                while (*value == ' ' || *value == '\t') value++;
                char* valueEnd = value + strlen(value) - 1;
                while (valueEnd > value && (*valueEnd == ' ' || *valueEnd == '\t')) {
                    *valueEnd = '\0';
                    valueEnd--;
                }
                
                if (iniKey == key) {
                    bool result = (strcmp(value, "1") == 0);
                    fclose(file);
                    return result;
                }
            }
        }
    }
    
    fclose(file);
    
    // Key not found — return caller-supplied default.
    return defaultValue;
}

int MiscGui::getConfigIntValue(const std::string& iniKey, int defaultValue)
{
    FILE* file = fopen("/config/sys-clk/config.ini", "r");
    if (!file) {
        return defaultValue;
    }
    
    char line[512];
    bool inValuesSection = false;

    size_t len;

    while (fgets(line, sizeof(line), file)) {
        // Remove newline if present
        len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Trim whitespace
        char* start = line;
        while (*start == ' ' || *start == '\t') start++;
        char* end = start + strlen(start) - 1;
        while (end > start && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        // Check for [values] section
        if (strcmp(start, "[values]") == 0) {
            inValuesSection = true;
            continue;
        }
        
        // Check for new section
        if (strlen(start) > 0 && start[0] == '[') {
            inValuesSection = false;
            continue;
        }
        
        // Parse key=value in values section
        if (inValuesSection) {
            char* equalPos = strchr(start, '=');
            if (equalPos != nullptr) {
                *equalPos = '\0'; // Split the string
                char* key = start;
                char* value = equalPos + 1;
                
                // Trim key
                char* keyEnd = key + strlen(key) - 1;
                while (keyEnd > key && (*keyEnd == ' ' || *keyEnd == '\t')) {
                    *keyEnd = '\0';
                    keyEnd--;
                }
                
                // Trim value
                while (*value == ' ' || *value == '\t') value++;
                char* valueEnd = value + strlen(value) - 1;
                while (valueEnd > value && (*valueEnd == ' ' || *valueEnd == '\t')) {
                    *valueEnd = '\0';
                    valueEnd--;
                }
                
                if (iniKey == key) {
                    int result = atoi(value);
                    fclose(file);
                    return result;
                }
            }
        }
    }
    
    fclose(file);
    return defaultValue;
}

void MiscGui::setConfigValue(const std::string& iniKey, bool value)
{
    // Read the entire file
    FILE* file = fopen("/config/sys-clk/config.ini", "r");
    std::vector<std::string> lines;
    
    if (file) {
        char line[512];
        size_t len;
        while (fgets(line, sizeof(line), file)) {
            // Remove newline if present
            len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            lines.push_back(std::string(line));
        }
        fclose(file);
    }
    
    // Find and update the value
    bool inValuesSection = false;
    bool keyFound = false;
    int valuesSectionIndex = -1;

    std::string trimmedLine;
    size_t equalPos;
    std::string key;
    for (size_t i = 0; i < lines.size(); i++) {
        trimmedLine = lines[i];
        trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
        trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);
        
        if (trimmedLine == "[values]") {
            inValuesSection = true;
            valuesSectionIndex = i;
            continue;
        }
        
        if (trimmedLine.length() > 0 && trimmedLine[0] == '[') {
            inValuesSection = false;
            continue;
        }
        
        if (inValuesSection) {
            equalPos = trimmedLine.find('=');
            if (equalPos != std::string::npos) {
                key = trimmedLine.substr(0, equalPos);
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                
                if (key == iniKey) {
                    lines[i] = iniKey + "=" + (value ? "1" : "0");
                    keyFound = true;
                    break;
                }
            }
        }
    }
    
    // If key wasn't found, add it to the values section
    if (!keyFound) {
        if (valuesSectionIndex == -1) {
            // Add [values] section if it doesn't exist
            lines.push_back("[values]");
            lines.push_back(iniKey + "=" + (value ? "1" : "0"));
        } else {
            // Add to existing values section
            lines.insert(lines.begin() + valuesSectionIndex + 1, iniKey + "=" + (value ? "1" : "0"));
        }
    }
    
    // Write the file back
    FILE* outFile = fopen("/config/sys-clk/config.ini", "w");
    if (outFile) {
        for (const auto& fileLine : lines) {
            fprintf(outFile, "%s\n", fileLine.c_str());
        }
        fclose(outFile);
    }
}

void MiscGui::setConfigIntValue(const std::string& iniKey, int value)
{
    // Read the entire file
    FILE* file = fopen("/config/sys-clk/config.ini", "r");
    std::vector<std::string> lines;
    
    if (file) {
        char line[512];
        size_t len;
        while (fgets(line, sizeof(line), file)) {
            // Remove newline if present
            len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            lines.push_back(std::string(line));
        }
        fclose(file);
    }
    
    // Find and update the value
    bool inValuesSection = false;
    bool keyFound = false;
    int valuesSectionIndex = -1;
    
    std::string trimmedLine;
    size_t equalPos;
    std::string key;

    for (size_t i = 0; i < lines.size(); i++) {
        trimmedLine = lines[i];
        trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
        trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);
        
        if (trimmedLine == "[values]") {
            inValuesSection = true;
            valuesSectionIndex = i;
            continue;
        }
        
        if (trimmedLine.length() > 0 && trimmedLine[0] == '[') {
            inValuesSection = false;
            continue;
        }
        
        if (inValuesSection) {
            equalPos = trimmedLine.find('=');
            if (equalPos != std::string::npos) {
                key = trimmedLine.substr(0, equalPos);
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                
                if (key == iniKey) {
                    lines[i] = iniKey + "=" + std::to_string(value);
                    keyFound = true;
                    break;
                }
            }
        }
    }
    
    // If key wasn't found, add it to the values section
    if (!keyFound) {
        if (valuesSectionIndex == -1) {
            // Add [values] section if it doesn't exist
            lines.push_back("[values]");
            lines.push_back(iniKey + "=" + std::to_string(value));
        } else {
            // Add to existing values section
            lines.insert(lines.begin() + valuesSectionIndex + 1, iniKey + "=" + std::to_string(value));
        }
    }
    
    // Write the file back
    FILE* outFile = fopen("/config/sys-clk/config.ini", "w");
    if (outFile) {
        for (const auto& fileLine : lines) {
            fprintf(outFile, "%s\n", fileLine.c_str());
        }
        fclose(outFile);
    }
}

// ---------------------------------------------------------------------------
// [overlay] section helpers — read/write the [overlay] INI section
// ---------------------------------------------------------------------------

bool MiscGui::getOverlayConfigValue(const std::string& iniKey, bool defaultValue)
{
    FILE* file = fopen("/config/sys-clk/config.ini", "r");
    if (!file)
        return defaultValue;

    char line[512];
    bool inOverlaySection = false;
    bool result = defaultValue;

    while (fgets(line, sizeof(line), file)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';

        char* start = line;
        while (*start == ' ' || *start == '\t') start++;
        char* end = start + strlen(start) - 1;
        while (end > start && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }

        if (strcmp(start, "[overlay]") == 0) { inOverlaySection = true; continue; }
        if (strlen(start) > 0 && start[0] == '[') { inOverlaySection = false; continue; }

        if (inOverlaySection) {
            char* equalPos = strchr(start, '=');
            if (equalPos) {
                *equalPos = '\0';
                char* key = start;
                char* value = equalPos + 1;
                char* ke = key + strlen(key) - 1;
                while (ke > key && (*ke == ' ' || *ke == '\t')) { *ke = '\0'; ke--; }
                while (*value == ' ' || *value == '\t') value++;
                if (iniKey == key) {
                    result = (strcmp(value, "1") == 0);
                    break;
                }
            }
        }
    }
    fclose(file);
    return result;
}

void MiscGui::setOverlayConfigValue(const std::string& iniKey, bool value)
{
    FILE* file = fopen("/config/sys-clk/config.ini", "r");
    std::vector<std::string> lines;

    if (file) {
        char line[512];
        while (fgets(line, sizeof(line), file)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
            lines.push_back(std::string(line));
        }
        fclose(file);
    }

    bool inOverlaySection = false;
    bool keyFound = false;
    int overlaySectionIndex = -1;

    for (size_t i = 0; i < lines.size(); i++) {
        std::string trimmed = lines[i];
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

        if (trimmed == "[overlay]") { inOverlaySection = true; overlaySectionIndex = (int)i; continue; }
        if (!trimmed.empty() && trimmed[0] == '[') { inOverlaySection = false; continue; }

        if (inOverlaySection) {
            size_t eq = trimmed.find('=');
            if (eq != std::string::npos) {
                std::string k = trimmed.substr(0, eq);
                k.erase(0, k.find_first_not_of(" \t"));
                k.erase(k.find_last_not_of(" \t") + 1);
                if (k == iniKey) {
                    lines[i] = iniKey + "=" + (value ? "1" : "0");
                    keyFound = true;
                    break;
                }
            }
        }
    }

    if (!keyFound) {
        if (overlaySectionIndex == -1) {
            lines.push_back("[overlay]");
            lines.push_back(iniKey + "=" + (value ? "1" : "0"));
        } else {
            lines.insert(lines.begin() + overlaySectionIndex + 1, iniKey + "=" + (value ? "1" : "0"));
        }
    }

    FILE* outFile = fopen("/config/sys-clk/config.ini", "w");
    if (outFile) {
        for (const auto& fileLine : lines)
            fprintf(outFile, "%s\n", fileLine.c_str());
        fclose(outFile);
    }
}

void MiscGui::addConfigToggle(const std::string& iniKey, const char* displayName) {
    tsl::elm::MiniToggleListItem* toggle = new tsl::elm::MiniToggleListItem(displayName, configValues[iniKey]);
    toggle->setStateChangedListener([this, iniKey](bool state) {
        configValues[iniKey] = state;
        setConfigValue(iniKey, state);
        this->lastContextUpdate = armGetSystemTick();
    });
    this->listElement->addItem(toggle);
    this->configToggles[iniKey] = toggle;
}

void MiscGui::updateConfigToggles() {
    for (const auto& [key, toggle] : this->configToggles) {
        if (toggle != nullptr) {
            bool currentValue = getConfigValue(key);
            configValues[key] = currentValue;
            toggle->setState(currentValue);
        }
    }
}

static constexpr int numEntries = 25;

void MiscGui::listUI()
{
    auto* moduleHeader = new tsl::elm::CategoryHeader("Settings");
    // HOC mode: show the kip version (e.g. "HOC 2.4.2") read from hoc.kip's
    // CUST block, or "No KIP" when the kip is missing / not applied.
    if (isUsingHOC) {
        const std::string kipVer = hocKipVersion();
        if (kipLoaded(true) && !kipVer.empty())
            moduleHeader->setValue("HOC " + kipVer, tsl::sectionTextColor);
        else
            moduleHeader->setValue("No KIP", tsl::warningTextColor);
    }
    this->listElement->addItem(moduleHeader);

    this->enabledToggle = new tsl::elm::ToggleListItem("Enable", false);
    enabledToggle->setStateChangedListener([this](bool state) {
        Result rc = sysclkIpcSetEnabled(state);
        if(R_FAILED(rc))
        {
            FatalGui::openWithResultCode("sysclkIpcSetEnabled", rc);
        }

        this->lastContextUpdate = armGetSystemTick();
        this->context->enabled = state;
    });
    this->listElement->addItem(this->enabledToggle);

    // HOC Toolkit launcher — only shown when running HOC and the flag file is present
    //if (isUsingHOC && ult::isFile("/switch/.packages/HOC Toolkit/flags/SHOW_IN_SYSCLK.flag")) {
    //    auto* hocItem = new tsl::elm::ListItem("HOC Toolkit", ult::DROPDOWN_SYMBOL);
    //    hocItem->setClickListener([](u64 keys) -> bool {
    //        if (keys & KEY_A) {
    //            tsl::setNextOverlay(ult::OVERLAY_PATH + "ovlmenu.ovl", "--direct --comboReturn --comboReturnFrom sys-clk-overlay.ovl --package HOC Toolkit");
    //            tsl::Overlay::get()->close();
    //            return true;
    //        }
    //        return false;
    //    });
    //    this->listElement->addItem(hocItem);
    //}

    // add gap
    this->listElement->addItem(new tsl::elm::CustomDrawer(
        [](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            // Empty drawer - just creates space
        }
    ), 12);

    // ── HOC-specific section ─────────────────────────────────────────
    if (usingHOC()) {
        // Allow Governing — HOC only, not present on EOS.
        auto* govToggle = new tsl::elm::MiniToggleListItem("Allow Governing", configValues["allow_governing"]);
        govToggle->setStateChangedListener([this](bool state) {
            configValues["allow_governing"] = state;
            setConfigValue("allow_governing", state);
            this->lastContextUpdate = armGetSystemTick();
            // Defer the swapTo to update() — calling it here (inside onClick's
            // call chain) would destroy 'this' before onClick finishes, causing a crash.
            this->m_pendingGovSwap = true;
        });
        this->listElement->addItem(govToggle);
        this->configToggles["allow_governing"] = govToggle;

        // CPU Governor Minimum Frequency — HOC only.
        // Values mirror hoc-clk: 510 → 1020 MHz in 102 MHz steps.
        // Stored in config.ini as raw Hz (e.g. 612000000).
        // Only shown when Allow Governing is enabled.
        if (configValues["allow_governing"]) {
            // CPU Governor Minimum Frequency — HOC only. Real DVFS steps, 510 → 1581 MHz.
            this->cpuGovMinTrackbar = new tsl::elm::NamedStepTrackBar(
                "", CPU_GOV_MIN_LABELS, true, "CPU Gov Min Freq"
            );
            this->cpuGovMinTrackbar->disableClickAnimation();
            const int storedCpuGovMin = getConfigIntValue("cpu_gov_min_freq", 612000000);
            this->cpuGovMinTrackbar->setProgress(static_cast<u8>(
                govMinNearestIndex(kCpuGovMinHz, kCpuGovMinCount, storedCpuGovMin)));
            this->m_cpuGovMinWritten = storedCpuGovMin;
            this->cpuGovMinTrackbar->setValueChangedListener([this](u8 value) {
                const int idx = std::max(0, std::min(kCpuGovMinCount - 1, static_cast<int>(value)));
                const int hz  = kCpuGovMinHz[idx];
                this->m_cpuGovMinWritten = hz;
                setConfigIntValue("cpu_gov_min_freq", hz);
                this->lastContextUpdate = armGetSystemTick();
            });
            this->listElement->addItem(this->cpuGovMinTrackbar);

            // GPU Governor Minimum Frequency — HOC only. Real DVFS steps, 153 → 844 MHz.
            this->gpuGovMinTrackbar = new tsl::elm::NamedStepTrackBar(
                "", GPU_GOV_MIN_LABELS, true, "GPU Gov Min Freq"
            );
            this->gpuGovMinTrackbar->disableClickAnimation();
            const int storedGpuGovMin = getConfigIntValue("gpu_gov_min_freq", 76800000);
            this->gpuGovMinTrackbar->setProgress(static_cast<u8>(
                govMinNearestIndex(kGpuGovMinHz, kGpuGovMinCount, storedGpuGovMin)));
            this->m_gpuGovMinWritten = storedGpuGovMin;
            this->gpuGovMinTrackbar->setValueChangedListener([this](u8 value) {
                const int idx = std::max(0, std::min(kGpuGovMinCount - 1, static_cast<int>(value)));
                const int hz  = kGpuGovMinHz[idx];
                this->m_gpuGovMinWritten = hz;
                setConfigIntValue("gpu_gov_min_freq", hz);
                this->lastContextUpdate = armGetSystemTick();
            });
            this->listElement->addItem(this->gpuGovMinTrackbar);
        }
    }


    // Common toggles present in both HOC and EOS
    addConfigToggle("uncapped_clocks", "Uncapped Clocks");

    // "Boost GPU Override" toggle — key differs between EOS and HOC sysmodules
    if (usingEOS()) {
        addConfigToggle("boost_gpu_override", "Boost GPU Override"); // EOS key
        addConfigToggle("auto_cpu_boost", "Auto CPU Boost");
    } else {
        addConfigToggle("ow_boost", "Boost GPU Override");           // HOC key
    }

    addConfigToggle("reversenx_sync", "Sync ReverseNX");

    if (usingEOS()) {
        // ── EOS-specific section ─────────────────────────────────────────
        // EOS has no allow_governing or dvfs_mode.
        // It replaces dvfs_offset with auto_gpu_vmin (3-step) + gpu_vmin_offset.

        // Auto GPU Vmin: Off / Official Service / Hijack
        this->autoGPUVminTrackbar = new tsl::elm::NamedStepTrackBar("", {
            "Off",
            "Official Service",
            "Hijack"
        }, true, "Auto GPU Vmin");
        this->autoGPUVminTrackbar->disableClickAnimation();
        const int initAutoVmin = std::max(0, std::min(2, getConfigIntValue("auto_gpu_vmin", 1)));
        this->autoGPUVminTrackbar->setProgress(static_cast<u8>(initAutoVmin));
        this->autoGPUVminTrackbar->setValueChangedListener([this](u8 value) {
            setConfigIntValue("auto_gpu_vmin", static_cast<int>(std::min(static_cast<u8>(2), value)));
            this->lastContextUpdate = armGetSystemTick();
        });
        this->listElement->addItem(this->autoGPUVminTrackbar);

        // GPU Vmin Offset — EOS stores as "100 - index*5":
        //   index 0 → stored 100  (label "-100 mV"), index 20 → stored 0  (label "0 mV"),
        //   index 24 → stored -20 (label "+20 mV")
        this->gpuVminOffsetTrackbar = new tsl::elm::NamedStepTrackBar(
            "", {
                "-100 mV", "-95 mV", "-90 mV", "-85 mV", "-80 mV",
                "-75 mV",  "-70 mV", "-65 mV", "-60 mV", "-55 mV",
                "-50 mV",  "-45 mV", "-40 mV", "-35 mV", "-30 mV",
                "-25 mV",  "-20 mV", "-15 mV", "-10 mV", "-5 mV",
                "0 mV",    "+5 mV",  "+10 mV", "+15 mV", "+20 mV"
            }, true, "GPU Vmin Offset");

        const int eosStored = getConfigIntValue("gpu_vmin_offset", 0);
        // Recover index: index = (100 - stored) / 5
        const int eosIndex = std::max(0, std::min(numEntries - 1, (100 - eosStored) / 5));
        this->gpuVminOffsetTrackbar->setProgress(static_cast<u8>(eosIndex));
        this->m_eosVminOffsetWritten = eosStored;

        this->gpuVminOffsetTrackbar->setValueChangedListener([this](u8 value) {
            // EOS formula: stored = 100 - (index * 5)
            const int storedValue = 100 - (static_cast<int>(value) * 5);
            this->m_eosVminOffsetWritten = storedValue;
            setConfigIntValue("gpu_vmin_offset", storedValue);
            this->lastContextUpdate = armGetSystemTick();
        });
        this->gpuVminOffsetTrackbar->disableClickAnimation();
        this->listElement->addItem(this->gpuVminOffsetTrackbar);

    } else {

        // GPU DVFS toggle — HOC only
        addConfigToggle("dvfs_mode", "GPU DVFS");

        // GPU Vmin Offset: -100 mV to +20 mV in 5 mV steps (25 entries)
        // HOC stores actual signed mV: index=0 → -100, index=20 → 0, index=24 → +20
        this->gpuVminOffsetTrackbar = new tsl::elm::NamedStepTrackBar(
            "", {
                "-100 mV", "-95 mV", "-90 mV", "-85 mV", "-80 mV",
                "-75 mV",  "-70 mV", "-65 mV", "-60 mV", "-55 mV",
                "-50 mV",  "-45 mV", "-40 mV", "-35 mV", "-30 mV",
                "-25 mV",  "-20 mV", "-15 mV", "-10 mV", "-5 mV",
                "0 mV",    "+5 mV",  "+10 mV", "+15 mV", "+20 mV"
            }, true, "GPU Vmin Offset");
        this->gpuVminOffsetTrackbar->disableClickAnimation();
        
        // Read stored mV value and convert to trackbar index.
        // stored=-100 → index=0, stored=0 → index=20, stored=+20 → index=24
        const int storedGPUVminOffsetValue = getConfigIntValue("dvfs_offset", 0);
        const int trackbarIndex = std::max(0, std::min(numEntries - 1, (storedGPUVminOffsetValue + 100) / 5));
        this->gpuVminOffsetTrackbar->setProgress(static_cast<u8>(trackbarIndex));
        // Seed the write-cache so refresh() never treats the init value as "external"
        // and resets m_value out from under the user's first click.
        this->m_dvfsOffsetWritten = storedGPUVminOffsetValue;

        this->gpuVminOffsetTrackbar->setValueChangedListener([this](u8 value) {
            const int storedValue = (static_cast<int>(value) * 5) - 100;
            // Update write-cache BEFORE the file write so that the 60-frame refresh
            // cannot see a "changed" value and silently reset m_value mid-click-chain.
            this->m_dvfsOffsetWritten = storedValue;
            setConfigIntValue("dvfs_offset", storedValue);
            this->lastContextUpdate = armGetSystemTick();
        });
        this->listElement->addItem(this->gpuVminOffsetTrackbar);
    }
}

void MiscGui::update()
{
    BaseMenuGui::update();

    if (this->m_pendingGovSwap) {
        this->m_pendingGovSwap = false;

        auto* govToggle = this->configToggles["allow_governing"];
        const bool on = this->configValues["allow_governing"];

        if (on && this->cpuGovMinTrackbar == nullptr) {
            // Turning ON — create fresh trackbars and insert them after govToggle.
            const s32 govIdx = this->listElement->getIndexInList(govToggle);

            // CPU Gov Min Freq (inserted directly after govToggle).
            this->cpuGovMinTrackbar = new tsl::elm::NamedStepTrackBar(
                "", CPU_GOV_MIN_LABELS, true, "CPU Gov Min Freq"
            );
            const int storedCpu = getConfigIntValue("cpu_gov_min_freq", 612000000);
            this->cpuGovMinTrackbar->setProgress(static_cast<u8>(
                govMinNearestIndex(kCpuGovMinHz, kCpuGovMinCount, storedCpu)));
            this->m_cpuGovMinWritten = storedCpu;
            this->cpuGovMinTrackbar->setValueChangedListener([this](u8 value) {
                const int idx = std::max(0, std::min(kCpuGovMinCount - 1, static_cast<int>(value)));
                const int hz  = kCpuGovMinHz[idx];
                this->m_cpuGovMinWritten = hz;
                setConfigIntValue("cpu_gov_min_freq", hz);
                this->lastContextUpdate = armGetSystemTick();
            });
            this->listElement->addItem(this->cpuGovMinTrackbar, 0, govIdx + 1);

            // GPU Gov Min Freq (inserted directly after the CPU trackbar).
            this->gpuGovMinTrackbar = new tsl::elm::NamedStepTrackBar(
                "", GPU_GOV_MIN_LABELS, true, "GPU Gov Min Freq"
            );
            const int storedGpu = getConfigIntValue("gpu_gov_min_freq", 76800000);
            this->gpuGovMinTrackbar->setProgress(static_cast<u8>(
                govMinNearestIndex(kGpuGovMinHz, kGpuGovMinCount, storedGpu)));
            this->m_gpuGovMinWritten = storedGpu;
            this->gpuGovMinTrackbar->setValueChangedListener([this](u8 value) {
                const int idx = std::max(0, std::min(kGpuGovMinCount - 1, static_cast<int>(value)));
                const int hz  = kGpuGovMinHz[idx];
                this->m_gpuGovMinWritten = hz;
                setConfigIntValue("gpu_gov_min_freq", hz);
                this->lastContextUpdate = armGetSystemTick();
            });
            this->listElement->addItem(this->gpuGovMinTrackbar, 0, govIdx + 2);

        } else if (!on && this->cpuGovMinTrackbar != nullptr) {
            // Turning OFF — remove (and delete) both trackbars.
            this->listElement->removeItem(this->cpuGovMinTrackbar);
            this->cpuGovMinTrackbar = nullptr; // pointer is now owned/deleted by the list
            if (this->gpuGovMinTrackbar != nullptr) {
                this->listElement->removeItem(this->gpuGovMinTrackbar);
                this->gpuGovMinTrackbar = nullptr;
            }
        }
    }
}

void MiscGui::refresh() {
    BaseMenuGui::refresh();

    // Update the enabled toggle state
    if(this->context)
    {
        this->enabledToggle->setState(this->context->enabled);
    }

    // Update config values and toggle states every 60 frames (once per second at 60fps)
    if (this->context && ++frameCounter >= 60)
    {
        frameCounter = 0;
        updateConfigToggles();


        // Update Auto GPU Vmin trackbar (EOS only: auto_gpu_vmin, range 0-2)
        if (usingEOS()) {
            if (this->autoGPUVminTrackbar != nullptr) {
                const int v = std::max(0, std::min(2, getConfigIntValue("auto_gpu_vmin", 1)));
                this->autoGPUVminTrackbar->setProgress(static_cast<u8>(v));
            }
            // EOS GPU Vmin Offset: key=gpu_vmin_offset, formula stored=100-(index*5)
            if (this->gpuVminOffsetTrackbar != nullptr) {
                const int eosStored = getConfigIntValue("gpu_vmin_offset", 0);
                if (eosStored != this->m_eosVminOffsetWritten) {
                    const int idx = std::max(0, std::min(numEntries - 1, (100 - eosStored) / 5));
                    this->gpuVminOffsetTrackbar->setProgress(static_cast<u8>(idx));
                    this->m_eosVminOffsetWritten = eosStored;
                }
            }
        } else {
            // HOC GPU Vmin Offset (key: dvfs_offset, stored as signed mV)
            if (this->gpuVminOffsetTrackbar != nullptr) {
                const int storedGPUVminOffsetValue = getConfigIntValue("dvfs_offset", 0);
                // Only reposition the trackbar when the file value differs from what
                // we last wrote (i.e. an external process changed it).  Calling
                // setProgress() unconditionally resets m_value between rapid clicks,
                // causing the "snaps to a prior step" bug.
                if (storedGPUVminOffsetValue != this->m_dvfsOffsetWritten) {
                    const int trackbarIndex = std::max(0, std::min(numEntries - 1, (storedGPUVminOffsetValue + 100) / 5));
                    this->gpuVminOffsetTrackbar->setProgress(static_cast<u8>(trackbarIndex));
                    this->m_dvfsOffsetWritten = storedGPUVminOffsetValue;
                }
            }

            // HOC CPU Governor Minimum Frequency (key: cpu_gov_min_freq, stored as Hz)
            if (this->cpuGovMinTrackbar != nullptr) {
                const int storedCpuGovMin = getConfigIntValue("cpu_gov_min_freq", 612000000);
                if (storedCpuGovMin != this->m_cpuGovMinWritten) {
                    this->cpuGovMinTrackbar->setProgress(static_cast<u8>(
                        govMinNearestIndex(kCpuGovMinHz, kCpuGovMinCount, storedCpuGovMin)));
                    this->m_cpuGovMinWritten = storedCpuGovMin;
                }
            }

            // HOC GPU Governor Minimum Frequency (key: gpu_gov_min_freq, stored as Hz)
            if (this->gpuGovMinTrackbar != nullptr) {
                const int storedGpuGovMin = getConfigIntValue("gpu_gov_min_freq", 76800000);
                if (storedGpuGovMin != this->m_gpuGovMinWritten) {
                    this->gpuGovMinTrackbar->setProgress(static_cast<u8>(
                        govMinNearestIndex(kGpuGovMinHz, kGpuGovMinCount, storedGpuGovMin)));
                    this->m_gpuGovMinWritten = storedGpuGovMin;
                }
            }
        }
    }
}