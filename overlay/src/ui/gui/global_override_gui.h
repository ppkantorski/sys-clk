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
#include <functional>

#ifndef SYSCLK_GLOBAL_PROFILE_TID
#define SYSCLK_GLOBAL_PROFILE_TID  0xA111111111111111ULL
#endif

class GlobalOverrideGui : public BaseMenuGui
{
    protected:
        tsl::elm::ListItem* listItems[SysClkModule_EnumMax];
        std::uint32_t listHz[SysClkModule_EnumMax];
        // Tracks the temp governor packed value across submenu visits.
        uint32_t m_tempGovernorPacked = 0;
        // Governor dropdown list item — kept so its label can be updated when
        // the user changes a bar inside GovernorOverrideSubMenuGui.
        tsl::elm::ListItem* m_governorItem = nullptr;
        // Optional callback fired with true/false on every IPC write so the
        // parent MainGui item can switch between DROPDOWN and INPROGRESS immediately.
        std::function<void(bool)> m_onStateChanged;

        void openFreqChoiceGui(SysClkModule module);
        void addModuleListItem(SysClkModule module);
        // Returns true when any temporary override (freq or HOC+governing governor) is set.
        bool hasAnyNonZero() const;

    public:
        explicit GlobalOverrideGui(std::function<void(bool)> onStateChanged = nullptr);
        ~GlobalOverrideGui() {}
        void listUI() override;
        void refresh() override;
};
