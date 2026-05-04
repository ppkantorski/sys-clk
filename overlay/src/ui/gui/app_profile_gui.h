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

#include "../../ipc.h"
#include "base_menu_gui.h"
#include "freq_choice_gui.h"

#define SYSCLK_GLOBAL_PROFILE_TID  0xA111111111111111ULL

class AppProfileGui : public BaseMenuGui
{
    protected:
        std::uint64_t applicationId;
        SysClkTitleProfileList* profileList;
        SysClkProfile m_initialProfile;
        // Per-profile governor packed values (HOC mode only).
        // Mirrors hoc-clk's profileList->mhzMap[profile][HocClkModule_Governor].
        // Fetched via IPC on changeTo; sent back via IPC when the user changes a bar.
        SysClkProfileGovernorList m_governors;

        void openFreqChoiceGui(tsl::elm::ListItem* listItem, SysClkProfile profile, SysClkModule module);
        void addModuleListItem(SysClkProfile profile, SysClkModule module);
        void addGovernorSection(SysClkProfile profile);
        void addProfileUI(SysClkProfile profile);

    public:
        AppProfileGui(std::uint64_t applicationId, SysClkTitleProfileList* profileList,
                      SysClkProfileGovernorList governors,
                      SysClkProfile initialProfile = SysClkProfile_Handheld);
        ~AppProfileGui();
        void listUI() override;
        static void changeTo(std::uint64_t applicationId,
                             SysClkProfile initialProfile = SysClkProfile_Handheld);
        void update() override;
};