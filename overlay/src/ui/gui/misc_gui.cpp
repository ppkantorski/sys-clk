#include "misc_gui.h"
#include "fatal_gui.h"
#include "../format.h"
#include <fstream>
#include <sstream>

MiscGui::MiscGui()
{
    
    // Load current config values
    configValues["uncapped_clocks"] = getConfigValue("uncapped_clocks");
    configValues["override_boost_mode"] = getConfigValue("override_boost_mode");
    configValues["auto_cpu_boost"] = getConfigValue("auto_cpu_boost");
    configValues["sync_reversenx"] = getConfigValue("sync_reversenx");
    configValues["gpu_dvfs"] = getConfigValue("gpu_dvfs");
}

MiscGui::~MiscGui()
{
    this->configToggles.clear();
}

bool MiscGui::getConfigValue(const std::string& iniKey)
{
    std::ifstream file("/config/sys-clk/config.ini");
    if (!file.is_open()) {
        // Return default values if file doesn't exist
        return (iniKey == "gpu_dvfs"); // gpu_dvfs defaults to true, others default to false
    }
    
    std::string line;
    bool inValuesSection = false;
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Check for [values] section
        if (line == "[values]") {
            inValuesSection = true;
            continue;
        }
        
        // Check for new section
        if (line.length() > 0 && line[0] == '[') {
            inValuesSection = false;
            continue;
        }
        
        // Parse key=value in values section
        if (inValuesSection) {
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);
                
                // Trim key and value
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                if (key == iniKey) {
                    return (value == "1");
                }
            }
        }
    }
    
    // Return default values if key not found
    return (iniKey == "gpu_dvfs"); // gpu_dvfs defaults to true, others default to false
}

void MiscGui::setConfigValue(const std::string& iniKey, bool value)
{
    // Read the entire file
    std::ifstream file("/config/sys-clk/config.ini");
    std::vector<std::string> lines;
    std::string line;
    
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();
    
    // Find and update the value
    bool inValuesSection = false;
    bool keyFound = false;
    int valuesSectionIndex = -1;
    
    for (size_t i = 0; i < lines.size(); i++) {
        std::string trimmedLine = lines[i];
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
            size_t equalPos = trimmedLine.find('=');
            if (equalPos != std::string::npos) {
                std::string key = trimmedLine.substr(0, equalPos);
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
    std::ofstream outFile("/config/sys-clk/config.ini");
    for (const auto& fileLine : lines) {
        outFile << fileLine << "\n";
    }
    outFile.close();
}

void MiscGui::addConfigToggle(const std::string& iniKey, const char* displayName) {
    tsl::elm::ToggleListItem* toggle = new tsl::elm::ToggleListItem(displayName, configValues[iniKey]);
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

    // Add the 5 specific config toggles using INI keys
    addConfigToggle("uncapped_clocks", "Uncapped Clocks");
    addConfigToggle("override_boost_mode", "Override Boost Mode");
    addConfigToggle("auto_cpu_boost", "Auto CPU Boost");
    addConfigToggle("sync_reversenx", "Sync ReverseNX");
    addConfigToggle("gpu_dvfs", "GPU DVFS");
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
    }
}
