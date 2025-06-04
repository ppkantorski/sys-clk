// Additional config settings designed for retronx-team's sys-clk 2.0.1

#pragma once

#include "../../ipc.h"
#include "base_menu_gui.h"
#include <unordered_map>
#include <string>

class MiscGui : public BaseMenuGui
{
    public:
        MiscGui();
        ~MiscGui();
        void listUI() override;
        void refresh() override;

    protected:
        
        std::unordered_map<std::string, tsl::elm::ToggleListItem*> configToggles;
        std::unordered_map<std::string, bool> configValues;
        
        void addConfigToggle(const std::string& iniKey, const char* displayName);
        void updateConfigToggles();
        bool getConfigValue(const std::string& iniKey);
        void setConfigValue(const std::string& iniKey, bool value);
        
        tsl::elm::ToggleListItem* enabledToggle; // Add this line
        
        u8 frameCounter = 60;
};
