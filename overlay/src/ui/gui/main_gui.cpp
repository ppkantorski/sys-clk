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
    bool isUsingHOC = usingHOC();

    if (!isUsingHOC) {
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

    this->listElement->addItem(new tsl::elm::CategoryHeader("System Profiles"));

    tsl::elm::ListItem* appProfileItem = new tsl::elm::ListItem("Edit App Profile");
    appProfileItem->setValue(ult::DROPDOWN_SYMBOL);
    appProfileItem->setClickListener([this, appProfileItem](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
        {
            tsl::shiftItemFocus(appProfileItem);
            AppProfileGui::changeTo(this->context->applicationId);
            return true;
        }

        return false;
    });
    this->listElement->addItem(appProfileItem);

    if (isUsingHOC) {
        tsl::elm::ListItem* globalProfileItem = new tsl::elm::ListItem("Edit Global Profile");
        globalProfileItem->setValue(ult::DROPDOWN_SYMBOL);
        globalProfileItem->setClickListener([this, globalProfileItem](u64 keys) {
            if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
            {
                tsl::shiftItemFocus(globalProfileItem);
                AppProfileGui::changeTo(SYSCLK_GLOBAL_PROFILE_TID);
                return true;
            }

            return false;
        });
        this->listElement->addItem(globalProfileItem);
    }

    this->listElement->addItem(new tsl::elm::CategoryHeader("Advanced"));

    tsl::elm::ListItem* globalOverrideItem = new tsl::elm::ListItem("Temporary Override");
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



    

    //this->listElement->addItem(new tsl::elm::CategoryHeader("Misc"));

    if (isUsingHOC) {

        tsl::elm::ListItem* miscItem = new tsl::elm::ListItem("Module Settings");
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
    static bool isUsingHOC = usingHOC();
    BaseMenuGui::refresh();
    if(!isUsingHOC && this->context) {
        this->enabledToggle->setState(this->context->enabled);
    }
}
