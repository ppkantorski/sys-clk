/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "main_gui.h"

#include "fatal_gui.h"
#include "app_profile_gui.h"
#include "global_override_gui.h"
#include "misc_gui.h"

void MainGui::listUI()
{
    // Both HOC and EOS share the same main menu structure:
    //   - Enable toggle lives in Settings (MiscGui), not on the main page
    //   - Global profile item is available
    //   - Advanced / Settings section is shown
    // Stock (neither HOC nor EOS) shows the Enable toggle on the main page
    // and has no Global profile or Settings entry.
    const bool isHOCLike = usingHOC() || usingEOS();

    if (!isHOCLike) {
        this->enabledToggle = new tsl::elm::ToggleListItem("Enable", false);
        enabledToggle->setStateChangedListener([this](bool state) {
            Result rc = sysclkIpcSetEnabled(state);
            if(R_FAILED(rc))
            {
                FatalGui::openWithResultCode("sysclkIpcSetEnabled", rc);
            }

            this->lastContextUpdate = armGetSystemTick();
            this->context->enabled = state;
        });
        this->listElement->addItem(this->enabledToggle);
    }

    this->listElement->addItem(new tsl::elm::CategoryHeader("Edit Profile"));

    tsl::elm::ListItem* appProfileItem = new tsl::elm::ListItem("Active App");
    appProfileItem->setValue(ult::DROPDOWN_SYMBOL);
    appProfileItem->setClickListener([this, appProfileItem](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
        {
            tsl::shiftItemFocus(appProfileItem);
            AppProfileGui::changeTo(this->context->applicationId, this->context->profile);
            return true;
        }

        return false;
    });
    this->listElement->addItem(appProfileItem);

    if (isHOCLike) {
        tsl::elm::ListItem* globalProfileItem = new tsl::elm::ListItem("Global");
        globalProfileItem->setValue(ult::DROPDOWN_SYMBOL);
        globalProfileItem->setClickListener([this, globalProfileItem](u64 keys) {
            if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
            {
                tsl::shiftItemFocus(globalProfileItem);
                AppProfileGui::changeTo(SYSCLK_GLOBAL_PROFILE_TID, this->context->profile);
                return true;
            }

            return false;
        });
        this->listElement->addItem(globalProfileItem);
    }

    tsl::elm::ListItem* globalOverrideItem = new tsl::elm::ListItem("Temporary");
    globalOverrideItem->setValue(ult::DROPDOWN_SYMBOL);
    globalOverrideItem->setClickListener([this, globalOverrideItem](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A)
        {
            tsl::shiftItemFocus(globalOverrideItem);
            tsl::changeTo<GlobalOverrideGui>();
            return true;
        }

        return false;
    });
    this->listElement->addItem(globalOverrideItem);

    if (isHOCLike) {
        this->listElement->addItem(new tsl::elm::CategoryHeader("Advanced"));

        tsl::elm::ListItem* miscItem = new tsl::elm::ListItem("Settings");
        miscItem->setValue(ult::DROPDOWN_SYMBOL);
        miscItem->setClickListener([this, miscItem](u64 keys) {
            if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
            {
                tsl::shiftItemFocus(miscItem);
                tsl::changeTo<MiscGui>();
                return true;
            }

            return false;
        });
        this->listElement->addItem(miscItem);
    }
}

void MainGui::refresh()
{
    static bool isHOCLike = usingHOC() || usingEOS();
    BaseMenuGui::refresh();
    if(!isHOCLike && this->context) {
        this->enabledToggle->setState(this->context->enabled);
    }
}