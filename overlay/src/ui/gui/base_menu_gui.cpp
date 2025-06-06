/*
 * --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */


#include "base_menu_gui.h"
#include "fatal_gui.h"

bool IsErista() { 
    SetSysProductModel model = SetSysProductModel_Invalid;
    setsysGetProductModel(&model);
    
    if (model == SetSysProductModel_Nx || \
        model == SetSysProductModel_Copper)
        return true;
    
    return false;
}
    
    
bool IsMariko() {
    SetSysProductModel model = SetSysProductModel_Invalid;
    setsysGetProductModel(&model);
    
    if (model == SetSysProductModel_Iowa || \
        model == SetSysProductModel_Hoag || \
        model == SetSysProductModel_Calcio || \
        model == SetSysProductModel_Aula)
        return true;
    
    return false;
}


BaseMenuGui::BaseMenuGui()
{
    tsl::initializeThemeVars();
    this->context = nullptr;
    this->lastContextUpdate = 0;
    this->listElement = nullptr;
    this->cpuVoltageUv = 0;
    this->gpuVoltageUv = 0;
    this->emcVoltageUv = 0;
    this->socVoltageUv = 0;
    this->vddVoltageUv = 0; 
}

BaseMenuGui::~BaseMenuGui()
{
    if(this->context)
    {
        delete this->context;
    }
}

void BaseMenuGui::preDraw(tsl::gfx::Renderer* renderer)
{
    BaseGui::preDraw(renderer);
    if(this->context)
    {
        char buf[32];
        std::uint32_t y = 95-4;
        renderer->drawRoundedRect(12+1,y-21,420,30,10.0,renderer->a(tsl::tableBGColor));
        renderer->drawString("App ID ", false, 22+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        snprintf(buf, sizeof(buf), "%016lX", context->applicationId);
        renderer->drawString(buf, false, 81+1, y, SMALL_TEXT_SIZE, tsl::infoTextColor);

        renderer->drawString("Profile ", false, 246+6+4+2+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        renderer->drawString(sysclkFormatProfile(context->profile, true), false, 302-2+6+4+1, y, SMALL_TEXT_SIZE, tsl::infoTextColor);

        y += 38;

        renderer->drawRoundedRect(12+1,y-26+1+2,420,118-2,10.0, renderer->a(tsl::tableBGColor));

        static struct
        {
            SysClkModule m;
            std::uint32_t x;
        } freqOffsets[SysClkModule_EnumMax] = {
            { SysClkModule_CPU, 61 +1},
            { SysClkModule_GPU, 204-4 -2+1},
            { SysClkModule_MEM, 342-4 +1},
        };

        for(unsigned int i = 0; i < SysClkModule_EnumMax; i++)
        {
            std::uint32_t hz = this->context->freqs[freqOffsets[i].m];
            snprintf(buf, sizeof(buf), "%u.%u MHz", hz / 1000000, hz / 100000 - hz / 1000000 * 10);
            renderer->drawString(buf, false, freqOffsets[i].x, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        }
        renderer->drawString("CPU", false, 22+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        renderer->drawString("GPU", false, 162-4+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        renderer->drawString("MEM", false, 295-1+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);

        y += 20;

        for(unsigned int i = 0; i < SysClkModule_EnumMax; i++)
        {
            std::uint32_t hz = this->context->realFreqs[freqOffsets[i].m];
            snprintf(buf, sizeof(buf), "%u.%u MHz", hz / 1000000, hz / 100000 - hz / 1000000 * 10);
            renderer->drawString(buf, false, freqOffsets[i].x, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        }

        y += 20;


        // CPU voltage
        snprintf(buf, sizeof(buf), "%u mV", cpuVoltageUv / 1000);
        renderer->drawString(buf, false, freqOffsets[0].x, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        
        // GPU voltage
        snprintf(buf, sizeof(buf), "%u mV", gpuVoltageUv / 1000);
        renderer->drawString(buf, false, freqOffsets[1].x, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        
        if (vddVoltageUv != 0) {
            // MEM voltage |VDDQ/VDD2
            snprintf(buf, sizeof(buf), "%u%u mV", emcVoltageUv / 1000, vddVoltageUv / 1000);
            renderer->drawStringWithColoredSections(buf, {""}, freqOffsets[2].x-19-4, y, SMALL_TEXT_SIZE, tsl::infoTextColor, tsl::separatorColor);
        } else {
            snprintf(buf, sizeof(buf), "%u mV", emcVoltageUv / 1000);
            renderer->drawString(buf, false, freqOffsets[2].x, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        }
        
        y += 22;
        
        static struct
        {
            SysClkThermalSensor s;
            std::uint32_t x;
        } tempOffsets[SysClkModule_EnumMax] = {
            { SysClkThermalSensor_SOC, 61 +1},
            { SysClkThermalSensor_PCB, 204-4 -2 +1},
            { SysClkThermalSensor_Skin, 342-4  +1},
        };

        renderer->drawString("SOC", false, 22+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        renderer->drawString("PCB", false, 166-4+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        renderer->drawString("Skin", false, 303-1+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        for(unsigned int i = 0; i < SysClkModule_EnumMax; i++)
        {
            std::uint32_t millis = this->context->temps[tempOffsets[i].s];
            snprintf(buf, sizeof(buf), "%u.%u °C", millis / 1000, (millis - millis / 1000 * 1000) / 100);
            
            // Convert millis to Celsius for color calculation
            float tempCelsius = static_cast<float>(millis) / 1000.0f;
            tsl::Color tempColor = tsl::GradientColor(tempCelsius);
            renderer->drawString(buf, false, tempOffsets[i].x, y, SMALL_TEXT_SIZE, tempColor);
            //renderer->drawString(buf, false, tempOffsets[i].x, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        }

        // soc voltage
        y += 20;

        if (socVoltageUv != 0) {
            snprintf(buf, sizeof(buf), "%u mV", socVoltageUv / 1000);
            renderer->drawString(buf, false, 61+1, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        }

        //y += 22;

        static struct
        {
            SysClkPowerSensor s;
            std::uint32_t x;
        } powerOffsets[SysClkPowerSensor_EnumMax] = {
            { SysClkPowerSensor_Now, 204 -2-2+1},
            { SysClkPowerSensor_Avg, 342 -2+1},
        };

        //renderer->drawString("Battery Power", false, 22+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);

        renderer->drawString("Now", false, 160-4+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        renderer->drawString("Avg", false, 304-1+1, y, SMALL_TEXT_SIZE, tsl::sectionTextColor);
        for(unsigned int i = 0; i < SysClkPowerSensor_EnumMax; i++)
        {
            std::uint32_t mw = this->context->power[powerOffsets[i].s];
            snprintf(buf, sizeof(buf), "%d mW", mw);
            renderer->drawString(buf, false, powerOffsets[i].x-2, y, SMALL_TEXT_SIZE, tsl::infoTextColor);
        }
    }
}

void BaseMenuGui::refresh()
{
    std::uint64_t ticks = armGetSystemTick();
    // Only run once every 1 000 000 000 ns (1 second)
    if (armTicksToNs(ticks - this->lastContextUpdate) > 1'000'000'000UL)
    {
        this->lastContextUpdate = ticks;
        if (!this->context)
        {
            this->context = new SysClkContext;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // Initialize the regulator library once, then read all relevant voltages
        // ─────────────────────────────────────────────────────────────────────────
        rgltrInitialize();

        //
        // Define the “full” list of 5 domains in .rodata (no stack cost):
        //
        static const PowerDomainId domainIdsAll[5] = {
            PcvPowerDomainId_Max77621_Cpu,    // CPU voltage
            PcvPowerDomainId_Max77621_Gpu,    // GPU voltage
            PcvPowerDomainId_Max77812_Dram,   // EMC/DRAM voltage
            PcvPowerDomainId_Max77620_Sd0,    // SOC (Sd0) voltage
            PcvPowerDomainId_Max77620_Sd1     // VDD2 (Sd1) voltage
        };
        //
        // Pointers to your five u32 members in BaseMenuGui:
        //   cpuVoltageUv, gpuVoltageUv, emcVoltageUv, socVoltageUv, vddVoltageUv
        //
        u32* voltagePtrsAll[5] = {
            &cpuVoltageUv,  // index 0 → CPU
            &gpuVoltageUv,  // index 1 → GPU
            &emcVoltageUv,  // index 2 → DRAM
            &socVoltageUv,  // index 3 → SOC
            &vddVoltageUv   // index 4 → VDD2
        };

        //
        // If we’re on Erista, skip the last two domains (Sd0 and Sd1).
        // Otherwise, use all five domains.
        //
        const int domainCount = IsMariko() ? 5 : 3;

        for (int i = 0; i < domainCount; i++)
        {
            // 1) Zero the output variable up front
            *(voltagePtrsAll[i]) = 0;

            // 2) Create a fresh session struct for each domain:
            RgltrSession session = {};

            // 3) Try to open a session on domainIdsAll[i]:
            if (R_SUCCEEDED(rgltrOpenSession(&session, domainIdsAll[i])))
            {
                // 4) If open succeeded, attempt to read voltage:
                if (R_FAILED(rgltrGetVoltage(&session, voltagePtrsAll[i])))
                {
                    // On failure to read, leave *(voltagePtrsAll[i]) == 0
                    *(voltagePtrsAll[i]) = 0;
                }

                // 5) Close the session before moving on
                rgltrCloseSession(&session);
            }
            // If rgltrOpenSession fails, the voltage stays at 0 automatically
        }

        rgltrExit();

        // ─────────────────────────────────────────────────────────────────────────
        // Now update the SysClkContext exactly as before
        // ─────────────────────────────────────────────────────────────────────────
        Result rc = sysclkIpcGetCurrentContext(this->context);
        if (R_FAILED(rc))
        {
            FatalGui::openWithResultCode("sysclkIpcGetCurrentContext", rc);
            return;
        }
    }
}

tsl::elm::Element* BaseMenuGui::baseUI()
{
    tsl::elm::List* list = new tsl::elm::List();
    this->listElement = list;
    this->listUI();

    return list;
}
