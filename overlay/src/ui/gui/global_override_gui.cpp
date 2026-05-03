/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "global_override_gui.h"

#include "fatal_gui.h"
#include "../format.h"
#include "labels.h"
#include "freq_choice_gui.h"

// HocClkModule_Governor = 3 — send via sysclkIpcSetOverride to set temporary governor.
#define GOVERNOR_MODULE_INDEX 3

// ── GovernorOverrideSubMenuGui ────────────────────────────────────────────
// Initialized with the parent GlobalOverrideGui's current packed value.
// On change, calls setter() to update the parent's member and the sysmodule.

class GovernorOverrideSubMenuGui : public BaseMenuGui {
    uint32_t packed;
    std::function<void(uint32_t)> setter;
public:
    GovernorOverrideSubMenuGui(uint32_t initialPacked, std::function<void(uint32_t)> setter)
        : packed(initialPacked), setter(std::move(setter)) {}

    void listUI() override {
        auto* header = new tsl::elm::CategoryHeader("Governor");
        header->setValue("Temporary " + ult::DIVIDER_SYMBOL + " Override", tsl::sectionTextColor);
        this->listElement->addItem(header);

        static constexpr struct { const char* label; int shift; } kAll[] = {
            { "CPU", 0 },
            { "GPU", 8 },
        };

        for (int i = 0; i < 2; i++) {
            u8 cur = (this->packed >> kAll[i].shift) & 0xFF;
            if (cur > 2) cur = 0;

            auto* bar = new tsl::elm::NamedStepTrackBar(
                "", { "Do not override", "Disabled", "Enabled" },
                true, kAll[i].label
            );
            bar->setProgress(cur);

            int shift = kAll[i].shift;
            bar->setValueChangedListener([this, shift](u8 value) {
                this->packed = (this->packed & ~(0xFFu << shift))
                              | ((uint32_t)value << shift);
                this->setter(this->packed);
            });

            this->listElement->addItem(bar);
        }
    }
};

// ── GlobalOverrideGui ─────────────────────────────────────────────────────

GlobalOverrideGui::GlobalOverrideGui()
{
    for(std::uint16_t m = 0; m < SysClkModule_EnumMax; m++)
    {
        this->listItems[m] = nullptr;
        this->listHz[m] = 0;
    }
}

void GlobalOverrideGui::openFreqChoiceGui(SysClkModule module)
{
    std::uint32_t hzList[SYSCLK_FREQ_LIST_MAX];
    std::uint32_t hzCount;
    Result rc = sysclkIpcGetFreqList(module, &hzList[0], SYSCLK_FREQ_LIST_MAX, &hzCount);
    if(R_FAILED(rc))
    {
        FatalGui::openWithResultCode("sysclkIpcGetFreqList", rc);
        return;
    }

    std::map<uint32_t, std::string> govLabels;
    if (usingHOC() && FreqChoiceGui::readShowGoverning()) {
        if (module == SysClkModule_CPU)
            govLabels = IsMariko() ? cpu_freq_label_m : cpu_freq_label_e;
        else if (module == SysClkModule_GPU)
            govLabels = IsMariko() ? gpu_freq_label_m : gpu_freq_label_e;
    }

    // SysClkProfile_EnumMax signals "Override" context to FreqChoiceGui
    tsl::changeTo<FreqChoiceGui>(this->context->overrideFreqs[module], hzList, hzCount,
        module, SysClkProfile_EnumMax, false,
        [this, module](std::uint32_t hz) {
        Result rc = sysclkIpcSetOverride(module, hz);
        if(R_FAILED(rc))
        {
            FatalGui::openWithResultCode("sysclkIpcSetOverride", rc);
            return false;
        }

        this->lastContextUpdate = armGetSystemTick();
        this->context->overrideFreqs[module] = hz;

        return true;
    }, govLabels);
}

void GlobalOverrideGui::addModuleListItem(SysClkModule module)
{
    tsl::elm::ListItem* listItem = new tsl::elm::ListItem(sysclkFormatModule(module, true));
    listItem->setValue(formatListFreqMHz(0));
    listItem->setClickListener([this, listItem, module](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A)
        {
            tsl::shiftItemFocus(listItem);
            this->openFreqChoiceGui(module);
            return true;
        }
        else if((keys & KEY_Y) == KEY_Y)
        {
            Result rc = sysclkIpcSetOverride(module, 0);
            if(R_FAILED(rc))
            {
                FatalGui::openWithResultCode("sysclkIpcSetOverride", rc);
                return false;
            }

            this->lastContextUpdate = armGetSystemTick();
            this->context->overrideFreqs[module] = 0;
            this->listHz[module] = 0;
            this->listItems[module]->setValue(formatListFreqHz(0));

            listItem->triggerClickAnimation();
            triggerSettingsFeedback();

            return true;
        }
        return false;
    });
    this->listElement->addItem(listItem);
    this->listItems[module] = listItem;
}

void GlobalOverrideGui::listUI()
{
    auto* header = new tsl::elm::CategoryHeader("Override " + ult::DIVIDER_SYMBOL + "  Reset");
    header->setValue("Temporary", tsl::sectionTextColor);
    this->listElement->addItem(header);

    this->addModuleListItem(SysClkModule_CPU);
    this->addModuleListItem(SysClkModule_GPU);
    this->addModuleListItem(SysClkModule_MEM);

    // Governor override — HOC mode + Allow Governing only
    if (usingHOC() && FreqChoiceGui::readShowGoverning()) {
        auto* item = new tsl::elm::ListItem("Governor");
        item->setValue(ult::DROPDOWN_SYMBOL);
        item->setClickListener([this, item](u64 keys) -> bool {
            if ((keys & HidNpadButton_A) == HidNpadButton_A) {
                tsl::shiftItemFocus(item);
                // Read current governor override from the sysmodule context —
                // same way CPU/GPU/MEM overrides are read back.
                if (this->context)
                    this->m_tempGovernorPacked = this->context->governorOverride;
                tsl::changeTo<GovernorOverrideSubMenuGui>(
                    this->m_tempGovernorPacked,
                    [this](uint32_t packed) {
                        this->m_tempGovernorPacked = packed;
                        sysclkIpcSetOverride((SysClkModule)GOVERNOR_MODULE_INDEX, packed);
                    });
                return true;
            }
            return false;
        });
        this->listElement->addItem(item);
    }
}

void GlobalOverrideGui::refresh()
{
    BaseMenuGui::refresh();
    if(this->context)
    {
        for(std::uint16_t m = 0; m < SysClkModule_EnumMax; m++)
        {
            if(this->listItems[m] != nullptr && this->listHz[m] != this->context->overrideFreqs[m])
            {
                this->listItems[m]->setValue(formatListFreqHz(this->context->overrideFreqs[m]));
                this->listHz[m] = this->context->overrideFreqs[m];
            }
        }
    }
}
