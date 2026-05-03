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

#include <list>
#include <map>

#include "base_menu_gui.h"

using FreqChoiceListener = std::function<bool(std::uint32_t hz)>;

#define FREQ_DEFAULT_TEXT "Do not override"

class FreqChoiceGui : public BaseMenuGui
{
    protected:
        std::uint32_t selectedHz;
        std::uint32_t* hzList;
        std::uint32_t hzCount;
        SysClkModule module;
        SysClkProfile profile;   // which profile row we came from
        bool isGlobal;           // true = global TID, false = app-specific
        FreqChoiceListener listener;

        // Governing (safety coloring + freq annotations) — HOC mode only.
        // Set at construction from show_governing in [overlay] config section.
        bool showGoverning;

        // Frequency annotation labels (e.g. "Safe Max", "Stock").
        // Populated by the caller (AppProfileGui) when governing is active.
        std::map<uint32_t, std::string> labels;

        // Create a single list item. safety: 0=ok, 1=warning, 2=danger.
        tsl::elm::ListItem* createFreqListItem(std::uint32_t hz, bool selected, int safety = 0);

        // Compute safety level for the given module + MHz value.
        static int computeSafety(SysClkModule module, uint32_t mhz);

    public:
        // Read show_governing from the [overlay] section of config.ini.
        // Public so AppProfileGui and GlobalOverrideGui can call it to decide
        // whether to build a label map before opening this screen.
        static bool readShowGoverning();
        FreqChoiceGui(std::uint32_t selectedHz,
                      std::uint32_t* hzList,
                      std::uint32_t hzCount,
                      SysClkModule module,
                      SysClkProfile profile,
                      bool isGlobal,
                      FreqChoiceListener listener,
                      std::map<uint32_t, std::string> labels = {});

        ~FreqChoiceGui() {}
        void listUI() override;
};
