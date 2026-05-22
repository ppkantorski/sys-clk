/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "base_gui.h"

#include "../elements/base_frame.h"
#include "logo_rgba_bin.h"


#define LOGO_WIDTH 110
#define LOGO_HEIGHT 39
#define LOGO_X 18
#define LOGO_Y 21

#define LOGO_LABEL_X (LOGO_X + LOGO_WIDTH + 6)
#define LOGO_LABEL_Y 50
#define LOGO_LABEL_FONT_SIZE 28

#define VERSION_X (LOGO_LABEL_X + 110+8)
#define VERSION_Y LOGO_LABEL_Y-4
#define VERSION_FONT_SIZE 15

std::string getVersionString() {
    char buf[0x100] = "";  // 256 bytes — safe for any expected version string
    Result rc = sysclkIpcGetVersionString(buf, sizeof(buf));
    if (R_FAILED(rc) || buf[0] == '\0') {
        return "unknown";
    }
    return std::string(buf);
}


bool usingHOC() {
    const std::string versionString = getVersionString();
    // Detect HOC sysmodule (version string ends with "-hoc")
    return versionString.find("hoc") != std::string::npos;
}

bool usingEOS() {
    const std::string versionString = getVersionString();
    // Detect EOS sysmodule (version string ends with "-eos")
    return versionString.find("eos") != std::string::npos;
}

// Returns true if the kip file for the active mode exists at its expected path.
// HOC uses hoc.kip; EOS uses loader.kip.  A file that exists but was never
// added to hekate / loaded by the bootloader will pass this check alone —
// that is why the EMC freq check (kipEmcApplied) is also required.
static bool kipFileExists(bool isHOC) {
    const char* path = isHOC
        ? "sdmc:/atmosphere/kips/hoc.kip"
        : "sdmc:/atmosphere/kips/loader.kip";
    FILE* fp = fopen(path, "r");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

// Returns true if the kip was actually applied by the bootloader.
// Both hoc.kip and loader.kip (EOS) patch the pcv EMC frequency table so that
// clkrst exposes rates above the stock 1600 MHz ceiling.  Without the kip,
// clkrst never reports any EMC rate above 1,600,000,000 Hz regardless of
// whether the file is present on the SD card.
// Result is cached — kip application state cannot change without a reboot.
static bool kipEmcApplied() {
    static constexpr u32 STOCK_MAX_EMC_HZ = 1'600'000'000u;
    static constexpr u32 MAX_FREQ_COUNT   = 32u;

    // −1 = unchecked, 0 = not applied, 1 = applied.
    static int cached = -1;
    if (cached != -1)
        return cached == 1;

    u32 freqList[MAX_FREQ_COUNT] = {};
    u32 outCount = 0;

    if (R_FAILED(sysclkIpcGetFreqList(SysClkModule_MEM, freqList, MAX_FREQ_COUNT, &outCount))
            || outCount == 0) {
        // Cannot determine — assume applied to avoid a false alarm.
        cached = 1;
        return true;
    }

    for (u32 i = 0; i < outCount; i++) {
        if (freqList[i] > STOCK_MAX_EMC_HZ) {
            cached = 1;
            return true;
        }
    }

    cached = 0;
    return false;
}

// Public wrapper: kip is considered "loaded" only when the correct file exists
// on the SD card AND the bootloader actually applied it (EMC table is patched).
// isHOC selects which file path to check (hoc.kip vs loader.kip for EOS).
bool kipLoaded(bool isHOC) {
    return kipFileExists(isHOC) && kipEmcApplied();
}

void BaseGui::preDraw(tsl::gfx::Renderer* renderer)
{
    renderer->drawBitmap(LOGO_X, LOGO_Y, LOGO_WIDTH, LOGO_HEIGHT, logo_rgba_bin);
    renderer->drawString("overlay", false, LOGO_LABEL_X, LOGO_LABEL_Y, LOGO_LABEL_FONT_SIZE, TEXT_COLOR);
    renderer->drawString(TARGET_VERSION, false, VERSION_X, VERSION_Y, VERSION_FONT_SIZE, tsl::bannerVersionTextColor);
    // usingHOC() and usingEOS() are now mutually exclusive, so order doesn't matter.
    if (isUsingEOS) {
        renderer->drawString("EOS mode", false, VERSION_X+86, VERSION_Y, VERSION_FONT_SIZE, tsl::warningTextColor);
    }
    else if (isUsingHOC) {
        renderer->drawString("HOC mode", false, VERSION_X+86, VERSION_Y, VERSION_FONT_SIZE, 0xF0F0);
    }
}

tsl::elm::Element* BaseGui::createUI()
{
    isUsingHOC = usingHOC();
    isUsingEOS = usingEOS();

    // Warn the user once per session if they're running in HOC or EOS mode but
    // the kip is either missing from the SD card or was not applied by the
    // bootloader (e.g. present in /atmosphere/kips/ but not in hekate's list).
    // The icon picks up hoc.rgba or eos.rgba from the Ultrahand assets folder
    // if they exist, giving a branded look to match the active module.
    static bool kipWarningShown = false;
    if (!kipWarningShown && (isUsingHOC || isUsingEOS) && !kipLoaded(isUsingHOC)) {
        kipWarningShown = true;
        if (tsl::notification) {
            const std::string iconName = isUsingHOC ? "hoc" : "eos";
            tsl::notification->showNow("No kip detected!", 26, "sys-clk-"+iconName, 4000, true, iconName);
        }
    }

    BaseFrame* rootFrame = new BaseFrame(this);
    rootFrame->setContent(this->baseUI());
    return rootFrame;
}

void BaseGui::update()
{
    this->refresh();
}