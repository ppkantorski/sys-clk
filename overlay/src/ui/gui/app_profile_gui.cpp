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
// Governor config.ini helpers
//
// sys-clk-hoc stores governor state under each app's TID section:
//   [{16-char uppercase hex TID}]
//   handheld_governor=1           ; packed u32: bits 7:0=CPU, bits 15:8=GPU
//   docked_governor=2             ; 0=Do not override, 1=Disabled, 2=Enabled
//
// The sysmodule reads/writes this on every profile update.  The overlay
// writes Governor directly to config.ini — the sysmodule picks it up on
// the next clock-manager tick.
// ---------------------------------------------------------------------------

static const char* profileIniName(SysClkProfile p)
{
    switch (p) {
    case SysClkProfile_Docked:                   return "docked";
    case SysClkProfile_Handheld:                 return "handheld";
    case SysClkProfile_HandheldCharging:         return "handheld_charging";
    case SysClkProfile_HandheldChargingUSB:      return "handheld_charging_usb";
    case SysClkProfile_HandheldChargingOfficial: return "handheld_charging_official";
    default:                                     return nullptr;
    }
}

static uint32_t readGovernorPacked(uint64_t tid, SysClkProfile profile)
{
    const char* pname = profileIniName(profile);
    if (!pname) return 0;

    char section[17];
    snprintf(section, sizeof(section), "%016llX", (unsigned long long)tid);

    char key[64];
    snprintf(key, sizeof(key), "%s_governor", pname);

    FILE* f = fopen("/config/sys-clk/config.ini", "r");
    if (!f) return 0;

    char line[256];
    bool inSection = false;
    uint32_t result = 0;

    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';

        char* p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == ';' || *p == '#') continue;

        if (*p == '[') {
            char sec[32] = {};
            strncpy(sec, p + 1, sizeof(sec) - 1);
            char* close = strchr(sec, ']');
            if (close) *close = '\0';
            inSection = (strcmp(sec, section) == 0);
            continue;
        }

        if (!inSection) continue;

        char* eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';

        char* ke = p + strlen(p) - 1;
        while (ke > p && (*ke == ' ' || *ke == '\t')) *ke-- = '\0';

        char* val = eq + 1;
        while (*val == ' ' || *val == '\t') ++val;

        if (strcmp(p, key) == 0) {
            result = (uint32_t)strtoul(val, nullptr, 10);
            break;
        }
    }

    fclose(f);
    return result;
}

static void writeGovernorPacked(uint64_t tid, SysClkProfile profile, uint32_t value)
{
    const char* pname = profileIniName(profile);
    if (!pname) return;

    char section[17];
    snprintf(section, sizeof(section), "%016llX", (unsigned long long)tid);

    char key[64];
    snprintf(key, sizeof(key), "%s_governor", pname);

    // Read full file
    FILE* f = fopen("/config/sys-clk/config.ini", "r");
    std::vector<std::string> lines;
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
            lines.push_back(std::string(line));
        }
        fclose(f);
    }

    bool inSection = false;
    bool keyFound  = false;
    int  secStart  = -1;

    for (size_t i = 0; i < lines.size(); i++) {
        std::string t = lines[i];
        t.erase(0, t.find_first_not_of(" \t"));
        if (t.find_last_not_of(" \t") != std::string::npos)
            t.erase(t.find_last_not_of(" \t") + 1);

        if (t.size() > 1 && t[0] == '[') {
            std::string sn = t.substr(1);
            auto cl = sn.find(']');
            if (cl != std::string::npos) sn = sn.substr(0, cl);
            inSection = (sn == section);
            if (inSection) secStart = (int)i;
            continue;
        }

        if (!inSection) continue;

        size_t eq = t.find('=');
        if (eq == std::string::npos) continue;

        std::string k = t.substr(0, eq);
        k.erase(0, k.find_first_not_of(" \t"));
        if (k.find_last_not_of(" \t") != std::string::npos)
            k.erase(k.find_last_not_of(" \t") + 1);

        if (k == key) {
            if (value == 0)
                lines.erase(lines.begin() + i);
            else
                lines[i] = std::string(key) + "=" + std::to_string(value);
            keyFound = true;
            break;
        }
    }

    if (!keyFound && value != 0) {
        if (secStart == -1) {
            lines.push_back(std::string("[") + section + "]");
            lines.push_back(std::string(key) + "=" + std::to_string(value));
        } else {
            lines.insert(lines.begin() + secStart + 1,
                         std::string(key) + "=" + std::to_string(value));
        }
    }

    FILE* out = fopen("/config/sys-clk/config.ini", "w");
    if (out) {
        for (const auto& l : lines)
            fprintf(out, "%s\n", l.c_str());
        fclose(out);
    }
}

// ---------------------------------------------------------------------------
// GovernorProfileSubMenuGui
// Shows CPU / GPU governor trackbars (Do not override / Disabled / Enabled).
// Reads and writes the packed u32 directly to config.ini, then immediately
// triggers a ForceRefresh+SetClocks cycle in the sysmodule via IPC so the
// change takes effect within one tick interval (~300 ms) without waiting
// for FAT mtime to advance (2-second resolution on SD cards).
// ---------------------------------------------------------------------------

class GovernorProfileSubMenuGui : public BaseMenuGui {
    uint64_t      applicationId;
    SysClkProfile profile;

public:
    GovernorProfileSubMenuGui(uint64_t appId, SysClkProfile prof)
        : applicationId(appId), profile(prof) {}

    void listUI() override {
        auto* header = new tsl::elm::CategoryHeader("Governor");

        char idLabel[20];
        if (this->applicationId == SYSCLK_GLOBAL_PROFILE_TID)
            strncpy(idLabel, "Global", sizeof(idLabel));
        else
            strncpy(idLabel, "App", sizeof(idLabel));

        header->setValue(std::string(idLabel) + " " + ult::DIVIDER_SYMBOL + " " + sysclkFormatProfile(this->profile, true), tsl::sectionTextColor);
        this->listElement->addItem(header);

        static constexpr struct { const char* label; int shift; } kAll[] = {
            { "CPU", 0 },
            { "GPU", 8 },
        };

        uint32_t packed = readGovernorPacked(this->applicationId, this->profile);

        for (int i = 0; i < 2; i++) {
            u8 cur = (packed >> kAll[i].shift) & 0xFF;
            if (cur > 2) cur = 0;

            auto* bar = new tsl::elm::NamedStepTrackBar(
                "", { "Do not override", "Disabled", "Enabled" },
                true, kAll[i].label
            );
            bar->setProgress(cur);

            int     shift  = kAll[i].shift;
            uint64_t tid   = this->applicationId;
            SysClkProfile prof = this->profile;

            bar->setValueChangedListener([tid, prof, shift](u8 value) {
                uint32_t p = readGovernorPacked(tid, prof);
                p = (p & ~(0xFFu << shift)) | ((uint32_t)value << shift);
                // Write to disk — sysmodule picks this up via Refresh() within
                // its normal polling interval.  Do NOT call sysclkIpcSetProfiles
                // here: ForceRefresh inside that handler races with this write and
                // can read a stale file, causing ini_putsection to erase the key.
                writeGovernorPacked(tid, prof, p);
            });

            this->listElement->addItem(bar);
        }
    }
};

// ---------------------------------------------------------------------------
// AppProfileGui
// ---------------------------------------------------------------------------

AppProfileGui::AppProfileGui(std::uint64_t applicationId, SysClkTitleProfileList* profileList)
{
    this->applicationId = applicationId;
    this->profileList   = profileList;
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

    // Governing annotation labels — HOC mode + show_governing only
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
    tsl::elm::ListItem* listItem =
        new tsl::elm::ListItem(sysclkFormatModule(module, true));
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
    // Only shown in HOC mode when Allow Governing is enabled
    if (!usingHOC() || !FreqChoiceGui::readShowGoverning())
        return;

    auto* item = new tsl::elm::ListItem("Governor");
    item->setValue(ult::DROPDOWN_SYMBOL);
    item->setClickListener([this, profile, item](u64 keys) -> bool {
        if ((keys & HidNpadButton_A) == HidNpadButton_A) {
            tsl::shiftItemFocus(item);
            tsl::changeTo<GovernorProfileSubMenuGui>(
                this->applicationId, profile
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
        strncpy(idLabel, "App", sizeof(idLabel));

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
}

void AppProfileGui::changeTo(std::uint64_t applicationId)
{
    SysClkTitleProfileList* profileList = new SysClkTitleProfileList;
    Result rc = sysclkIpcGetProfiles(applicationId, profileList);
    if(R_FAILED(rc))
    {
        delete profileList;
        FatalGui::openWithResultCode("sysclkIpcGetProfiles", rc);
        return;
    }

    tsl::changeTo<AppProfileGui>(applicationId, profileList);
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
