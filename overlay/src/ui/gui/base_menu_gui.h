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

#include "../../rgltr_services.h"
#include "../../ipc.h"
#include "base_gui.h"

class BaseMenuGui : public BaseGui
{
    protected:
        SysClkContext* context;
        std::uint64_t lastContextUpdate;
        std::uint32_t cpuVoltageUv;
        std::uint32_t gpuVoltageUv;
        std::uint32_t emcVoltageUv;
		std::uint32_t socVoltageUv; //add soc voltage
		std::uint32_t vddVoltageUv;//add vdd2 voltage

    public:
        BaseMenuGui();
        ~BaseMenuGui();
        void preDraw(tsl::gfx::Renderer* renderer) override;
        bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                         HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick) override;
        tsl::elm::List* listElement;
        tsl::elm::Element* baseUI() override;
        void refresh() override;
        virtual void listUI() = 0;

    private:
        char displayStrings[20][32];  // [0-16] existing, [17-19] CPU/GPU/MEM component temps
        tsl::Color tempColors[3];     // Pre-computed temperature colors
        bool isUsingEOS;
        // When true (HOC only), the CPU/GPU/MEM freq row shows per-component
        // die temperatures instead of target frequencies.  Toggled by Y button.
        // Static so the preference survives navigating away and back.
        static bool m_showComponentTemps;
};
