/* --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

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
        bool isMariko = false;
        
        std::unordered_map<std::string, tsl::elm::ToggleListItem*> configToggles;
        std::unordered_map<std::string, bool> configValues;
        
        void addConfigToggle(const std::string& iniKey, const char* displayName);
        void updateConfigToggles();
        bool getConfigValue(const std::string& iniKey);
        void setConfigValue(const std::string& iniKey, bool value);
        
        u8 frameCounter = 60;
};