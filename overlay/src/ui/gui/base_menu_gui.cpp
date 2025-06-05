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

    if(armTicksToNs(ticks - this->lastContextUpdate) > 1000000000UL)
    {
        this->lastContextUpdate = ticks;
        if(!this->context)
        {
            this->context = new SysClkContext;
        }

        // update voltage
        rgltrInitialize();
        RgltrSession rgltr = {};
        
        // CPU voltage
        cpuVoltageUv = 0;
        if (R_SUCCEEDED(rgltrOpenSession(&rgltr, PcvPowerDomainId_Max77621_Cpu))) {
            if (R_FAILED(rgltrGetVoltage(&rgltr, &cpuVoltageUv))) cpuVoltageUv = 0;
            rgltrCloseSession(&rgltr);
        }
        
        // GPU voltage
        rgltr = {};
        gpuVoltageUv = 0;
        if (R_SUCCEEDED(rgltrOpenSession(&rgltr, PcvPowerDomainId_Max77621_Gpu))) {
            if (R_FAILED(rgltrGetVoltage(&rgltr, &gpuVoltageUv))) gpuVoltageUv = 0;
            rgltrCloseSession(&rgltr);
        }
        
        // EMC voltage
        rgltr = {};
        emcVoltageUv = 0;
        if (R_SUCCEEDED(rgltrOpenSession(&rgltr, PcvPowerDomainId_Max77812_Dram))) {
            if (R_FAILED(rgltrGetVoltage(&rgltr, &emcVoltageUv))) emcVoltageUv = 0;
            rgltrCloseSession(&rgltr);
        }
        
        // New SOC voltage
        rgltr = {};
        socVoltageUv = 0;
        if (R_SUCCEEDED(rgltrOpenSession(&rgltr, PcvPowerDomainId_Max77620_Sd0))) {
            if (R_FAILED(rgltrGetVoltage(&rgltr, &socVoltageUv))) socVoltageUv = 0;
            rgltrCloseSession(&rgltr);
        }
        
        // New vdd2 voltage
        rgltr = {};
        vddVoltageUv = 0;
        if (R_SUCCEEDED(rgltrOpenSession(&rgltr, PcvPowerDomainId_Max77620_Sd1))) {
            if (R_FAILED(rgltrGetVoltage(&rgltr, &vddVoltageUv))) vddVoltageUv = 0;
            rgltrCloseSession(&rgltr);
        }
        rgltrExit();

        Result rc = sysclkIpcGetCurrentContext(this->context);
        if(R_FAILED(rc))
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
