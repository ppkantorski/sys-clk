/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "app_profile_gui.h"

#include "../format.h"
#include "fatal_gui.h"
#include "labels.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// GovernorProfileSubMenuGui
//
// Mirrors hoc-clk's design exactly:
//   - Holds a pointer to the parent AppProfileGui's m_governors array
//   - Reads and writes governor packed values in-place via IPC (no file I/O)
//   - Calls sysclkIpcSetProfileGovernors (HOC cmd 13) after every change
//
// The global allow_governing toggle is still read from config.ini via
// FreqChoiceGui::readShowGoverning() — it is a HOC-specific value not
// exposed through the stock sys-clk IPC config value list.
// ---------------------------------------------------------------------------

class GovernorProfileSubMenuGui : public BaseMenuGui {
    uint64_t               m_tid;
    SysClkProfileGovernorList* m_governors;  // pointer into AppProfileGui::m_governors
    SysClkProfile          m_profile;

public:
    GovernorProfileSubMenuGui(uint64_t tid, SysClkProfileGovernorList* governors, SysClkProfile profile)
        : m_tid(tid), m_governors(governors), m_profile(profile) {}

    void listUI() override {
        auto* header = new tsl::elm::CategoryHeader("Governor");
        char idLabel[20];
        if (m_tid == SYSCLK_GLOBAL_PROFILE_TID)
            strncpy(idLabel, "Global", sizeof(idLabel));
        else
            strncpy(idLabel, "Active App", sizeof(idLabel));
        header->setValue(std::string(idLabel) + " " + ult::DIVIDER_SYMBOL + " " +
                         sysclkFormatProfile(m_profile, true), tsl::sectionTextColor);
        this->listElement->addItem(header);

        static constexpr struct { const char* label; int shift; } kAll[] = {
            { "CPU",  0 },
            { "GPU",  8 },
        };

        uint32_t packed = m_governors->packed[m_profile];

        for (int i = 0; i < 2; i++) {
            u8 cur = (packed >> kAll[i].shift) & 0xFF;
            if (cur > 2) cur = 0;

            auto* bar = new tsl::elm::NamedStepTrackBar(
                "", { "Do not override", "Disabled", "Enabled" },
                true, kAll[i].label
            );
            bar->setProgress(cur);

            int    shift     = kAll[i].shift;
            uint64_t tid     = m_tid;
            SysClkProfileGovernorList* gov = m_governors;
            SysClkProfile prof = m_profile;

            bar->setValueChangedListener([tid, gov, prof, shift](u8 value) {
                // Update in-place (same pattern as hoc-clk's profileList->mhzMap[prof][Governor])
                uint32_t& packed = gov->packed[prof];
                packed = (packed & ~(0xFFu << shift)) | ((uint32_t)value << shift);
                // Push to sysmodule via IPC — no file writes needed
                sysclkIpcSetProfileGovernors(tid, gov);
            });

            this->listElement->addItem(bar);
        }
    }
};

// ---------------------------------------------------------------------------
// AppProfileGui
// ---------------------------------------------------------------------------

AppProfileGui::AppProfileGui(std::uint64_t applicationId, SysClkTitleProfileList* profileList,
                             SysClkProfileGovernorList governors, SysClkProfile initialProfile)
{
    this->applicationId  = applicationId;
    this->profileList    = profileList;
    this->m_governors    = governors;
    this->m_initialProfile = initialProfile;
}

AppProfileGui::~AppProfileGui()
{
    delete this->profileList;
}

void AppProfileGui::openFreqChoiceGui(tsl::elm::ListItem* listItem,
                                      SysClkProfile profile,
                                      SysClkModule module)
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

    tsl::shiftItemFocus(listItem);
    const bool isGlobal = (this->applicationId == SYSCLK_GLOBAL_PROFILE_TID);
    tsl::changeTo<FreqChoiceGui>(
        this->profileList->mhzMap[profile][module] * 1000000,
        hzList, hzCount, module, profile, isGlobal,
        [this, listItem, profile, module](std::uint32_t hz) {
            this->profileList->mhzMap[profile][module] = hz / 1000000;
            listItem->setValue(formatListFreqMHz(this->profileList->mhzMap[profile][module]));
            Result rc = sysclkIpcSetProfiles(this->applicationId, this->profileList);
            if(R_FAILED(rc)) {
                FatalGui::openWithResultCode("sysclkIpcSetProfiles", rc);
                return false;
            }
            return true;
        },
        govLabels
    );
}

void AppProfileGui::addModuleListItem(SysClkProfile profile, SysClkModule module)
{
    std::string label = sysclkFormatModule(module, true);
    if (module == SysClkModule_CPU)
        label += std::string("?") + sysclkFormatProfile(profile, false);

    tsl::elm::ListItem* listItem = new tsl::elm::ListItem(label);
    listItem->setValue(formatListFreqMHz(this->profileList->mhzMap[profile][module]));
    listItem->setClickListener([this, listItem, profile, module](u64 keys) {
        if((keys & KEY_A) == KEY_A)
        {
            this->openFreqChoiceGui(listItem, profile, module);
            return true;
        }
        else if((keys & KEY_Y) == KEY_Y)
        {
            this->profileList->mhzMap[profile][module] = 0;
            listItem->setValue(formatListFreqMHz(0));

            Result rc = sysclkIpcSetProfiles(this->applicationId, this->profileList);
            if(R_FAILED(rc))
            {
                FatalGui::openWithResultCode("sysclkIpcSetProfiles", rc);
                triggerSettingsFeedback();
                listItem->triggerClickAnimation();
                return false;
            }
            triggerSettingsFeedback();
            listItem->triggerClickAnimation();
            return true;
        }
        return false;
    });
    this->listElement->addItem(listItem);
}

void AppProfileGui::addGovernorSection(SysClkProfile profile)
{
    if (!usingHOC() || !FreqChoiceGui::readShowGoverning())
        return;

    auto* item = new tsl::elm::ListItem("Governor");
    item->setValue(ult::DROPDOWN_SYMBOL);
    item->setClickListener([this, profile, item](u64 keys) -> bool {
        if ((keys & HidNpadButton_A) == HidNpadButton_A) {
            tsl::shiftItemFocus(item);
            tsl::changeTo<GovernorProfileSubMenuGui>(
                this->applicationId, &this->m_governors, profile
            );
            return true;
        }
        return false;
    });
    this->listElement->addItem(item);
}

void AppProfileGui::addProfileUI(SysClkProfile profile)
{
    char idLabel[20];
    if (this->applicationId == SYSCLK_GLOBAL_PROFILE_TID)
        strncpy(idLabel, "Global", sizeof(idLabel));
    else
        strncpy(idLabel, "Active App", sizeof(idLabel));

    auto* header = new tsl::elm::CategoryHeader(
        sysclkFormatProfile(profile, true) + std::string(" ") +
        ult::DIVIDER_SYMBOL + "  Reset");
    header->setValue(idLabel, tsl::sectionTextColor);
    this->listElement->addItem(header);

    this->addModuleListItem(profile, SysClkModule_CPU);
    this->addModuleListItem(profile, SysClkModule_GPU);
    this->addModuleListItem(profile, SysClkModule_MEM);
    this->addGovernorSection(profile);
}

void AppProfileGui::listUI()
{
    this->addProfileUI(SysClkProfile_Docked);
    this->addProfileUI(SysClkProfile_Handheld);
    this->addProfileUI(SysClkProfile_HandheldCharging);
    this->addProfileUI(SysClkProfile_HandheldChargingOfficial);
    this->addProfileUI(SysClkProfile_HandheldChargingUSB);

    if (m_initialProfile < SysClkProfile_EnumMax) {
        std::string jumpTag = std::string("CPU?") + sysclkFormatProfile(m_initialProfile, false);
        this->listElement->jumpToItem(jumpTag, "", true);
    }
}

void AppProfileGui::changeTo(std::uint64_t applicationId, SysClkProfile initialProfile)
{
    SysClkTitleProfileList* profileList = new SysClkTitleProfileList;
    Result rc = sysclkIpcGetProfiles(applicationId, profileList);
    if(R_FAILED(rc))
    {
        delete profileList;
        FatalGui::openWithResultCode("sysclkIpcGetProfiles", rc);
        return;
    }

    // Fetch per-profile governor values in HOC mode (cmd 12).
    // In stock sys-clk mode the call returns an error and we just zero-init.
    SysClkProfileGovernorList governors = {};
    if (usingHOC()) {
        sysclkIpcGetProfileGovernors(applicationId, &governors);
        // Ignore R_FAILED — governors stays zero-initialised (no governor shown)
    }

    tsl::changeTo<AppProfileGui>(applicationId, profileList, governors, initialProfile);
}

void AppProfileGui::update()
{
    BaseMenuGui::update();

    if((this->context && this->applicationId != this->context->applicationId) &&
        this->applicationId != SYSCLK_GLOBAL_PROFILE_TID)
    {
        tsl::changeTo<FatalGui>(
            "Application changed\n\n"
            "\n"
            "The running application changed\n\n"
            "while editing was going on.",
            ""
        );
    }
}
