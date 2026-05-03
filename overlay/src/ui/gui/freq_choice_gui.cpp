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
#include <cstdio>
#include <cstring>

FreqChoiceGui::FreqChoiceGui(std::uint32_t selectedHz,
                             std::uint32_t* hzList,
                             std::uint32_t hzCount,
                             SysClkModule module,
                             SysClkProfile profile,
                             bool isGlobal,
                             FreqChoiceListener listener,
                             std::map<uint32_t, std::string> labels)
{
    this->selectedHz  = selectedHz;
    this->hzList      = hzList;
    this->hzCount     = hzCount;
    this->module      = module;
    this->profile     = profile;
    this->isGlobal    = isGlobal;
    this->listener    = listener;
    this->labels      = labels;

    // Governing coloring is only meaningful in HOC mode.
    // Read the toggle once at construction; stays stable for this screen.
    this->showGoverning = usingHOC() && readShowGoverning();
}

// ---------------------------------------------------------------------------
// Read allow_governing from the [values] section of /config/sys-clk/config.ini.
// This is the single source of truth shared with the sysmodule.
// ---------------------------------------------------------------------------
bool FreqChoiceGui::readShowGoverning()
{
    FILE* file = fopen("/config/sys-clk/config.ini", "r");
    if (!file)
        return false;

    char line[256];
    bool inValuesSection = false;
    bool result = false;

    while (fgets(line, sizeof(line), file)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';

        char* p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == ';' || *p == '#') continue;

        if (*p == '[') {
            inValuesSection = (strncmp(p, "[values]", 8) == 0);
            continue;
        }
        if (!inValuesSection) continue;

        char* eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';

        char* key = p;
        char* val = eq + 1;
        char* ke  = key + strlen(key) - 1;
        while (ke > key && (*ke == ' ' || *ke == '\t')) *ke-- = '\0';
        while (*val == ' ' || *val == '\t') ++val;

        if (strcmp(key, "allow_governing") == 0) {
            result = (strcmp(val, "1") == 0);
            break;
        }
    }

    fclose(file);
    return result;
}

// ---------------------------------------------------------------------------
// Safety level for a given module + MHz (conservative defaults, UV level 0).
//   0 = safe (white)   1 = warning (orange)   2 = danger (red)
// ---------------------------------------------------------------------------
int FreqChoiceGui::computeSafety(SysClkModule module, uint32_t mhz)
{
    if (module == SysClkModule_MEM)
        return 0;

    uint32_t unsafe_cpu, danger_cpu, unsafe_gpu, danger_gpu;

    if (IsMariko()) {
        unsafe_cpu = 1964;  danger_cpu = 2398;
        unsafe_gpu = 1076;  danger_gpu = 1306;
    } else {
        unsafe_cpu = 1786;  danger_cpu = 1964;
        unsafe_gpu =  922;  danger_gpu =  999;
    }

    if (module == SysClkModule_CPU) {
        if (mhz >= danger_cpu) return 2;
        if (mhz >= unsafe_cpu) return 1;
    } else if (module == SysClkModule_GPU) {
        if (mhz >= danger_gpu) return 2;
        if (mhz >= unsafe_gpu) return 1;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Build one list item — mirrors HOC freq_choice_gui behaviour:
//   • text color follows safety level
//   • right-side value: annotation label (grey), or selected checkmark
// ---------------------------------------------------------------------------
tsl::elm::ListItem* FreqChoiceGui::createFreqListItem(std::uint32_t hz,
                                                      bool selected,
                                                      int safety)
{
    // Look up annotation label for this frequency (if any)
    std::string rightText;
    if (this->showGoverning && hz != 0) {
        auto it = this->labels.find(hz);
        if (it != this->labels.end())
            rightText = it->second;
    }

    // Selected checkmark overrides the annotation
    if (selected)
        rightText = "\uE14B";

    tsl::elm::ListItem* listItem =
        new tsl::elm::ListItem(formatListFreqHz(hz), rightText, false);

    // Apply governing safety coloring (text + value), matching HOC exactly
    if (this->showGoverning && hz != 0) {
        switch (safety) {
        case 1:  // Warning — use theme warning color
            listItem->setTextColor(tsl::warningTextColor);
            listItem->setValueColor(tsl::warningTextColor);
            break;
        case 2:  // Danger — bright red
            listItem->setTextColor(tsl::Color(255, 0, 0, 255));
            listItem->setValueColor(tsl::Color(255, 0, 0, 255));
            break;
        default:  // Safe — standard white
            listItem->setTextColor(tsl::Color(255, 255, 255, 255));
            listItem->setValueColor(tsl::Color(255, 255, 255, 255));
            break;
        }
    }

    // Annotation label → offTextColor
    if (!rightText.empty() && !selected)
        listItem->setValueColor(tsl::offTextColor);

    // Selected checkmark → theme info color
    if (selected)
        listItem->setValueColor(tsl::infoTextColor);

    listItem->setClickListener([this, hz](u64 keys) {
        if ((keys & KEY_A) == KEY_A && this->listener) {
            if (this->listener(hz))
                tsl::goBack();
            return true;
        }
        return false;
    });

    return listItem;
}

void FreqChoiceGui::listUI()
{
    std::string moduleName = sysclkFormatModule(this->module, true);
    std::string title;
    if (this->profile == SysClkProfile_EnumMax) {
        title = "Temporary " + ult::DIVIDER_SYMBOL + "Override";
    } else {
        std::string scope = this->isGlobal ? "Global" : "App";
        title = scope + " " + ult::DIVIDER_SYMBOL + " " + sysclkFormatProfile(this->profile, true);
    }

    auto* header = new tsl::elm::CategoryHeader(moduleName);
    header->setValue(title, tsl::sectionTextColor);
    this->listElement->addItem(header);

    // "Do not override" — always safe/white, no label
    this->listElement->addItem(this->createFreqListItem(0, this->selectedHz == 0, 0));

    for (std::uint32_t i = 0; i < this->hzCount; i++) {
        std::uint32_t hz = this->hzList[i];

        if (moduleName == "Memory" && hz == 204000000)
            continue;

        int safety = 0;
        if (this->showGoverning) {
            safety = computeSafety(this->module, hz / 1000000);
        }

        this->listElement->addItem(
            this->createFreqListItem(
                hz,
                (hz / 1000000) == (this->selectedHz / 1000000),
                safety
            )
        );
    }

    this->listElement->jumpToItem("", "");
}
