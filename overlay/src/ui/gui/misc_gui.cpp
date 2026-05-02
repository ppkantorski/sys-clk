#include "misc_gui.h"
#include "fatal_gui.h"
#include "../format.h"
#include <cstdio>
#include <cstring>
//#include <sstream>

MiscGui::MiscGui()
{
    
    // Load current config values — keys must match HOC sysmodule's INI key names
    configValues["uncapped_clocks"] = getConfigValue("uncapped_clocks");
    configValues["ow_boost"]        = getConfigValue("ow_boost");           // HOC: ow_boost (was boost_gpu_override)
    configValues["auto_cpu_boost"]  = getConfigValue("auto_cpu_boost");
    configValues["reversenx_sync"]  = getConfigValue("reversenx_sync");     // consistent key name
    configValues["dvfs_mode"]       = getConfigValue("dvfs_mode", true);  // sysmodule default is ON
    // dvfs_offset handled separately as trackbars
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

    this->listElement->addItem(new tsl::elm::CategoryHeader("Settings"));

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

    // Add the 4 boolean config toggles using INI keys
    addConfigToggle("uncapped_clocks", "Uncapped Clocks");
    addConfigToggle("ow_boost",        "Boost GPU Override");  // HOC key: ow_boost
    addConfigToggle("auto_cpu_boost",  "Auto CPU Boost");
    addConfigToggle("reversenx_sync",  "Sync ReverseNX");
    
    // Add GPU DVFS mode as a 2-step trackbar: HOC only has Disabled(0) and Hijack(1)
    //this->autoGPUVminTrackbar = new tsl::elm::NamedStepTrackBar("", {
    //    "Off",
    //    "Hijack"
    //}, true, "Auto GPU Vmin");
    //
    //// Ensure the value is within valid range (0-1)
    //const int currentAutoGPUVminValue = std::max(0, std::min(1, getConfigIntValue("dvfs_mode", 1)));
    //this->autoGPUVminTrackbar->setProgress(static_cast<u8>(currentAutoGPUVminValue));
    //
    //// Set up the value change listener to update the INI file (HOC key: dvfs_mode)
    //this->autoGPUVminTrackbar->setValueChangedListener([this](u8 value) {
    //    const int intValue = static_cast<int>(std::min(static_cast<u8>(1), value));
    //    setConfigIntValue("dvfs_mode", intValue);
    //    this->lastContextUpdate = armGetSystemTick();
    //});
    //
    //this->listElement->addItem(this->autoGPUVminTrackbar);


    addConfigToggle("dvfs_mode",  "GPU DVFS");


    // GPU Vmin Offset: -100 mV to +20 mV in 5 mV steps (25 entries)
    // Stored in config as actual signed mV value: -100, -95, ..., 0, +5, ..., +20
    // HOC clock_manager reads dvfs_offset and adds it directly to vmin (in mV)
    this->gpuVminOffsetTrackbar = new tsl::elm::NamedStepTrackBar(
        "", 
        {
            "-100 mV",
            "-95 mV",
            "-90 mV",
            "-85 mV",
            "-80 mV",
            "-75 mV",
            "-70 mV",
            "-65 mV",
            "-60 mV",
            "-55 mV",
            "-50 mV",
            "-45 mV",
            "-40 mV",
            "-35 mV",
            "-30 mV",
            "-25 mV",
            "-20 mV",
            "-15 mV",
            "-10 mV",
            "-5 mV",
            "0 mV",
            "+5 mV",
            "+10 mV",
            "+15 mV",
            "+20 mV"
        },
        true,
        "GPU Vmin Offset"
    );

        
    // Read stored mV value and convert to trackbar index.
    // stored=-100 → index=0, stored=0 → index=20, stored=+20 → index=24
    const int storedGPUVminOffsetValue = getConfigIntValue("dvfs_offset", 0);
    const int trackbarIndex = std::max(0, std::min(numEntries-1, (storedGPUVminOffsetValue + 100) / 5));
    this->gpuVminOffsetTrackbar->setProgress(static_cast<u8>(trackbarIndex));
    // Seed the write-cache so refresh() never treats the init value as "external"
    // and resets m_value out from under the user's first click.
    this->m_dvfsOffsetWritten = storedGPUVminOffsetValue;

    // Write back actual signed mV value so HOC sysmodule applies it correctly.
    // index=0 → -100, index=20 → 0, index=24 → +20
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
        
        // Update Auto GPU Vmin trackbar (HOC key: dvfs_mode, range 0-1)
        //if (this->autoGPUVminTrackbar != nullptr) {
        //    const int currentAutoGPUVminValue = std::max(0, std::min(1, getConfigIntValue("dvfs_mode", 1)));
        //    this->autoGPUVminTrackbar->setProgress(static_cast<u8>(currentAutoGPUVminValue));
        //}

        // Update GPU Vmin Offset trackbar (HOC key: dvfs_offset, stored as signed mV)
        if (this->gpuVminOffsetTrackbar != nullptr) {
            const int storedGPUVminOffsetValue = getConfigIntValue("dvfs_offset", 0);
            // Only reposition the trackbar when the file value differs from what
            // we last wrote (i.e. an external process changed it).  Calling
            // setProgress() unconditionally resets m_value between rapid clicks,
            // causing the "snaps to a prior step" bug.
            if (storedGPUVminOffsetValue != this->m_dvfsOffsetWritten) {
                const int trackbarIndex = std::max(0, std::min(numEntries-1, (storedGPUVminOffsetValue + 100) / 5));
                this->gpuVminOffsetTrackbar->setProgress(static_cast<u8>(trackbarIndex));
                this->m_dvfsOffsetWritten = storedGPUVminOffsetValue;
            }
        }
    }
}