/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "freq_choice_gui.h"

#include "../format.h"
#include "fatal_gui.h"

FreqChoiceGui::FreqChoiceGui(std::uint32_t selectedHz, std::uint32_t* hzList, std::uint32_t hzCount,
                             SysClkModule module, SysClkProfile profile, bool isGlobal,
                             FreqChoiceListener listener)
{
    this->selectedHz = selectedHz;
    this->hzList = hzList;
    this->hzCount = hzCount;
    this->module = module;
    this->profile = profile;
    this->isGlobal = isGlobal;
    this->listener = listener;
}

tsl::elm::ListItem* FreqChoiceGui::createFreqListItem(std::uint32_t hz, bool selected)
{
    tsl::elm::ListItem* listItem = new tsl::elm::ListItem(formatListFreqHz(hz), "", true);
    listItem->setValue(selected ? "\uE14B" : "");

    listItem->setClickListener([this, hz](u64 keys) {
        if((keys & KEY_A) == KEY_A && this->listener)
        {
            if(this->listener(hz))
            {
                tsl::goBack();
            }
            return true;
        }

        return false;
    });

    return listItem;
}

void FreqChoiceGui::listUI()
{
    // Header title:
    //   Profile context : "Global ➟ Docked" / "App ➟ Handheld" etc.
    //   Override context: "Override"  (profile == SysClkProfile_EnumMax sentinel)
    // Header value (right-aligned): module name (CPU / GPU / Memory).
    std::string moduleName = sysclkFormatModule(this->module, true);
    std::string title;
    if (this->profile == SysClkProfile_EnumMax) {
        title = "Override";
    } else {
        std::string scope = this->isGlobal ? "Global" : "App";
        title = scope + " " + ult::DIVIDER_SYMBOL + " " + sysclkFormatProfile(this->profile, true);
    }

    auto* header = new tsl::elm::CategoryHeader(moduleName);
    header->setValue(title, tsl::sectionTextColor);
    this->listElement->addItem(header);

    this->listElement->addItem(this->createFreqListItem(0, this->selectedHz == 0));
    std::uint32_t hz;
    for(std::uint32_t i = 0; i < this->hzCount; i++) {
        hz = this->hzList[i];
        // Skip 204 MHz exactly
        if(moduleName == "Memory" && hz == 204000000) {
            continue;
        }

        this->listElement->addItem(this->createFreqListItem(hz, (hz / 1000000) == (this->selectedHz / 1000000)));
    }
    this->listElement->jumpToItem("", "");
}