#include "misc_gui.h"
#include "fatal_gui.h"
#include "../format.h"
#include <cstdio>
#include <cstring>
//#include <sstream>

// ── RefreshRateGui ────────────────────────────────────────────────────────

constexpr int RefreshRateGui::RATES[];

tsl::elm::ListItem* RefreshRateGui::createRateItem(int hz, bool selected)
{
    char label[16];
    snprintf(label, sizeof(label), "%d Hz", hz);
    tsl::elm::ListItem* item = new tsl::elm::ListItem(label, "", true);
    item->setValue(selected ? "\uE14B" : "");

    item->setClickListener([this, hz](u64 keys) -> bool {
        if ((keys & KEY_A) == KEY_A) {
            BaseMenuGui::applyRefreshRateHz(hz);
            // Notify the caller (MiscGui) immediately so its label updates
            // without waiting up to 60 frames for the periodic refresh().
            if (this->m_onSelected) this->m_onSelected(hz);
            tsl::goBack();
            return true;
        }
        return false;
    });
    return item;
}

void RefreshRateGui::listUI()
{
    auto* header = new tsl::elm::CategoryHeader("Overlay Settings");
    header->setValue("Refresh Rate", tsl::sectionTextColor);
    this->listElement->addItem(header);

    const int current = BaseMenuGui::getRefreshRateHz();
    char selectedLabel[16];
    snprintf(selectedLabel, sizeof(selectedLabel), "%d Hz", current);

    for (int i = 0; i < RATE_COUNT; i++) {
        this->listElement->addItem(createRateItem(RATES[i], RATES[i] == current));
    }
    // Jump cursor to the currently-selected item
    this->listElement->jumpToItem(selectedLabel, "");
}

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
    auto* moduleHeader = new tsl::elm::CategoryHeader("Module Settings");
    //moduleHeader->setValue("Settings", tsl::sectionTextColor);
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
        });
        this->listElement->addItem(govToggle);
        this->configToggles["allow_governing"] = govToggle;

        // CPU Governor Minimum Frequency — HOC only.
        // Values mirror hoc-clk: 510 → 1020 MHz in 102 MHz steps.
        // Stored in config.ini as raw Hz (e.g. 612000000).
        this->cpuGovMinTrackbar = new tsl::elm::NamedStepTrackBar(
            "", { "510 MHz", "612 MHz", "714 MHz", "816 MHz", "918 MHz", "1020 MHz" },
            true, "CPU Gov Min Freq"
        );

        const int storedCpuGovMin = getConfigIntValue("cpu_gov_min_freq", 612000000);
        const int cpuGovMinIndex  = std::max(0, std::min(5,
            (storedCpuGovMin / 1000000 - 510) / 102));
        this->cpuGovMinTrackbar->setProgress(static_cast<u8>(cpuGovMinIndex));
        this->m_cpuGovMinWritten = storedCpuGovMin;

        this->cpuGovMinTrackbar->setValueChangedListener([this](u8 value) {
            const int hz = (static_cast<int>(value) * 102 + 510) * 1000000;
            this->m_cpuGovMinWritten = hz;
            setConfigIntValue("cpu_gov_min_freq", hz);
            this->lastContextUpdate = armGetSystemTick();
        });
        this->listElement->addItem(this->cpuGovMinTrackbar);
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

    // ── Overlay Settings ─────────────────────────────────────────────────
    auto* overlayHeader = new tsl::elm::CategoryHeader("Overlay Settings");
    //overlayHeader->setValue("Settings", tsl::sectionTextColor);
    this->listElement->addItem(overlayHeader);

    // Refresh Rate dropdown item
    this->refreshRateItem = new tsl::elm::MiniListItem("Refresh Rate", ult::DROPDOWN_SYMBOL);
    {
        // Display the current rate as the item value
        char valStr[16];
        snprintf(valStr, sizeof(valStr), "%d Hz", BaseMenuGui::getRefreshRateHz());
        this->refreshRateItem->setValue(valStr);
    }
    this->refreshRateItem->setClickListener([this](u64 keys) -> bool {
        if ((keys & HidNpadButton_A) == HidNpadButton_A) {
            tsl::shiftItemFocus(this->refreshRateItem);
            // Capture the item pointer so the callback can update its label
            // the instant the user makes a selection — no waiting for refresh().
            auto* rateItem = this->refreshRateItem;
            tsl::changeTo<RefreshRateGui>([rateItem](int hz) {
                char valStr[16];
                snprintf(valStr, sizeof(valStr), "%d Hz", hz);
                rateItem->setValue(valStr);
            });
            return true;
        }
        return false;
    });
    this->listElement->addItem(this->refreshRateItem);
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

        // Keep the Refresh Rate item value display in sync (e.g. after returning
        // from RefreshRateGui or if the config was edited externally).
        if (this->refreshRateItem != nullptr) {
            char valStr[16];
            snprintf(valStr, sizeof(valStr), "%d Hz", BaseMenuGui::getRefreshRateHz());
            this->refreshRateItem->setValue(valStr);
        }
        
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
                    const int idx = std::max(0, std::min(5,
                        (storedCpuGovMin / 1000000 - 510) / 102));
                    this->cpuGovMinTrackbar->setProgress(static_cast<u8>(idx));
                    this->m_cpuGovMinWritten = storedCpuGovMin;
                }
            }
        }
    }
}