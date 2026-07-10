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
        tsl::Color tempColors[6];     // Pre-computed temperature colors
        // NOTE: isUsingEOS is inherited from BaseGui (public member).
        //       Do NOT redeclare it here — a private shadow would never be initialised
        //       and would cause EOS to be treated as stock throughout this class.
        // Non-HOC SOCTHERM die temperatures (milliCelsius).
        // Populated each refresh cycle via Soctherm::Read() when not in HOC mode.
        uint32_t componentCPU_mC;
        uint32_t componentGPU_mC;
        uint32_t componentRAM_mC;
        // Toggle state for the CPU/GPU/MEM top row:
        //   HOC  mode: false = target freqs (default), true  = HOC IPC component temps
        //   non-HOC:   true  = SOCTHERM temps (default),false = target freqs
        // Pressing + flips the state in both modes.
        // Static so the preference survives navigating away and back.
        static bool m_showComponentTemps;
        // Touch-tap detection: set true when a touch starts inside the data table rect.
        // The toggle fires on release (End attribute) if the flag is still set.
        bool m_touchStartedInRect;
};