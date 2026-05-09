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

// ---------------------------------------------------------------------------
// Helpers for computing initial INPROGRESS state
// ---------------------------------------------------------------------------

// Returns true if the given profile has any non-zero frequency saved for the
// given title, OR (HOC mode + Allow Governing only) a governor entry is set
// for that same profile.  Only the active profile is checked — changes to
// other profiles must not influence the Edit Profile symbol.
static bool profileHasAnyOverride(uint64_t tid, SysClkProfile profile)
{
    SysClkTitleProfileList profiles = {};
    if (R_FAILED(sysclkIpcGetProfiles(tid, &profiles)))
        return false;

    for (int m = 0; m < SysClkModule_EnumMax; m++)
        if (profiles.mhzMap[profile][m])
            return true;

    // Governor counts only in HOC mode with Allow Governing enabled.
    if (usingHOC() && FreqChoiceGui::readShowGoverning()) {
        SysClkProfileGovernorList governors = {};
        if (R_SUCCEEDED(sysclkIpcGetProfileGovernors(tid, &governors)))
            if (governors.packed[profile])
                return true;
    }

    return false;
}

// Returns true if any temporary override frequency is non-zero,
// OR (HOC mode + Allow Governing only) the governor override is set.
static bool tempOverrideActive(const SysClkContext& ctx)
{
    for (int m = 0; m < SysClkModule_EnumMax; m++)
        if (ctx.overrideFreqs[m])
            return true;

    if (usingHOC() && FreqChoiceGui::readShowGoverning())
        if (ctx.governorOverride)
            return true;

    return false;
}

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

    // ── Fetch initial context for symbol seeding ──────────────────────────
    // One cheap IPC call; failures leave items at DROPDOWN_SYMBOL (safe default).
    SysClkContext initCtx = {};
    bool hasInitCtx = R_SUCCEEDED(sysclkIpcGetCurrentContext(&initCtx));
    uint64_t initAppId = hasInitCtx ? initCtx.applicationId : 0;

    // ── Active App ────────────────────────────────────────────────────────
    tsl::elm::ListItem* appProfileItem = new tsl::elm::ListItem("Active App");
    this->m_appProfileItem = appProfileItem;

    // Seed the symbol: query the active app's saved profiles once.
    if (initAppId != 0)
        appProfileItem->setValue(profileHasAnyOverride(initAppId, initCtx.profile)
                                 ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
    else
        appProfileItem->setValue(ult::DROPDOWN_SYMBOL);

    appProfileItem->setClickListener([this, appProfileItem](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
        {
            tsl::shiftItemFocus(appProfileItem);
            // Callback fires on every value change inside AppProfileGui so the
            // symbol updates as soon as you alter a frequency or governor there.
            AppProfileGui::changeTo(
                this->context->applicationId,
                this->context->profile,
                [appProfileItem](bool active) {
                    appProfileItem->setValue(
                        active ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
                }
            );
            return true;
        }

        return false;
    });
    this->listElement->addItem(appProfileItem);

    // ── Global ────────────────────────────────────────────────────────────
    if (isHOCLike) {
        tsl::elm::ListItem* globalProfileItem = new tsl::elm::ListItem("Global");
        this->m_globalProfileItem = globalProfileItem;

        // Seed the symbol from the global profile's saved data.
        globalProfileItem->setValue(
            profileHasAnyOverride(SYSCLK_GLOBAL_PROFILE_TID, initCtx.profile)
            ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);

        globalProfileItem->setClickListener([this, globalProfileItem](u64 keys) {
            if((keys & HidNpadButton_A) == HidNpadButton_A && this->context)
            {
                tsl::shiftItemFocus(globalProfileItem);
                AppProfileGui::changeTo(
                    SYSCLK_GLOBAL_PROFILE_TID,
                    this->context->profile,
                    [globalProfileItem](bool active) {
                        globalProfileItem->setValue(
                            active ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
                    }
                );
                return true;
            }

            return false;
        });
        this->listElement->addItem(globalProfileItem);
    }

    // ── Temporary ─────────────────────────────────────────────────────────
    tsl::elm::ListItem* globalOverrideItem = new tsl::elm::ListItem("Temporary");
    this->m_globalOverrideItem = globalOverrideItem;

    // Seed the symbol from the current context.
    globalOverrideItem->setValue(
        (hasInitCtx && tempOverrideActive(initCtx))
        ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);

    globalOverrideItem->setClickListener([this, globalOverrideItem](u64 keys) {
        if((keys & HidNpadButton_A) == HidNpadButton_A)
        {
            tsl::shiftItemFocus(globalOverrideItem);
            // Callback fires on every IPC write inside GlobalOverrideGui so the
            // symbol updates the moment you change or clear an override there.
            tsl::changeTo<GlobalOverrideGui>([globalOverrideItem](bool active) {
                globalOverrideItem->setValue(
                    active ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
            });
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

    // Snapshot the tick before calling BaseMenuGui::refresh() so we can tell
    // whether it ran a full context update or took the early-return path.
    const u64 prevUpdateTick = this->lastContextUpdate;

    BaseMenuGui::refresh();

    if (!isHOCLike && this->context) {
        this->enabledToggle->setState(this->context->enabled);
    }

    if (!this->context) return;

    // Only re-evaluate Edit Profile symbols when the context was actually
    // refreshed this cycle.  Checking the tick avoids extra IPC calls on
    // every frame (BaseMenuGui guards refreshes to the configured interval).
    if (this->lastContextUpdate == prevUpdateTick) return;

    // Always evaluate against the current profile — this naturally handles
    // dock/undock, charger plug/unplug, and app switches without any
    // change-detection bookkeeping.
    if (this->m_appProfileItem && this->context->applicationId != 0) {
        this->m_appProfileItem->setValue(
            profileHasAnyOverride(this->context->applicationId, this->context->profile)
            ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
    }

    if (this->m_globalProfileItem) {
        this->m_globalProfileItem->setValue(
            profileHasAnyOverride(SYSCLK_GLOBAL_PROFILE_TID, this->context->profile)
            ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
    }

    if (this->m_globalOverrideItem) {
        this->m_globalOverrideItem->setValue(
            tempOverrideActive(*this->context)
            ? ult::INPROGRESS_SYMBOL : ult::DROPDOWN_SYMBOL);
    }
}