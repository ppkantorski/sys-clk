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

#include <tesla.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

#include "../style.h"
#include "../../ipc.h"

class BaseGui : public tsl::Gui
{
    public:
        BaseGui() {}
        ~BaseGui() {}
        virtual void preDraw(tsl::gfx::Renderer* renderer);
        void update() override;
        tsl::elm::Element* createUI() override;
        virtual tsl::elm::Element* baseUI() = 0;
        virtual void refresh() {}
        bool isUsingHOC;
        bool isUsingEOS;
    private:
};


extern bool usingHOC();
extern bool usingEOS();

// Returns true if the HOC or EOS kip is both present on the SD card and was
// actually applied by the bootloader (EMC frequency table has entries above
// the stock 1600 MHz ceiling).  Pass isHOC=true for hoc.kip, false for EOS
// loader.kip.  Result is cached after the first call per mode.
extern bool kipLoaded(bool isHOC);

// Hardware SoC detection (cached after first call)
extern bool IsMariko();
extern bool IsErista();