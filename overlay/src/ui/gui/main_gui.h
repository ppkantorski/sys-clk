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

#include "base_menu_gui.h"
#include "freq_choice_gui.h"

class MainGui : public BaseMenuGui
{
    protected:
        tsl::elm::ToggleListItem* enabledToggle;
        // Pointers kept so refresh() and nested-menu callbacks can update
        // the symbol between DROPDOWN and INPROGRESS without a full rebuild.
        tsl::elm::ListItem* m_appProfileItem    = nullptr;
        tsl::elm::ListItem* m_globalProfileItem = nullptr;
        tsl::elm::ListItem* m_globalOverrideItem = nullptr;

    public:
        MainGui() {}
        ~MainGui() {}
        void listUI() override;
        void refresh() override;
};