#pragma once
#include "../../ipc.h"
#include "base_menu_gui.h"
#include <unordered_map>
#include <string>
#include <vector>

// ── Refresh-Rate picker GUI ───────────────────────────────────────────────
// Shown when the user taps the "Refresh Rate" item in Overlay Settings.
class RefreshRateGui : public BaseMenuGui
{
public:
    // onSelected fires immediately when the user picks a rate, before goBack(),
    // so the caller can update its list-item label without waiting for refresh().
    RefreshRateGui(std::function<void(int)> onSelected = nullptr)
        : m_onSelected(std::move(onSelected)) {}
    ~RefreshRateGui() override {}
    void listUI() override;
    // No periodic refresh needed for this static list
    void refresh() override { BaseMenuGui::refresh(); }
private:
    static constexpr int RATES[] = {1, 2, 3, 5};
    static constexpr int RATE_COUNT = 7;
    tsl::elm::ListItem* createRateItem(int hz, bool selected);
    std::function<void(int)> m_onSelected;
};

class MiscGui : public BaseMenuGui
{
    public:
        MiscGui();
        ~MiscGui();
        void listUI() override;
        void refresh() override;
        void update() override;
    protected:
        
        std::unordered_map<std::string, tsl::elm::ToggleListItem*> configToggles;
        std::unordered_map<std::string, bool> configValues;
        
        void addConfigToggle(const std::string& iniKey, const char* displayName);
        void updateConfigToggles();
        bool getConfigValue(const std::string& iniKey, bool defaultValue = false);
        void setConfigValue(const std::string& iniKey, bool value);
        int getConfigIntValue(const std::string& iniKey, int defaultValue);
        void setConfigIntValue(const std::string& iniKey, int value);

        // [overlay] section helpers (HOC: show_governing, refresh_rate_hz, …)
        bool getOverlayConfigValue(const std::string& iniKey, bool defaultValue = false);
        void setOverlayConfigValue(const std::string& iniKey, bool value);
        
        tsl::elm::ToggleListItem* enabledToggle;
        tsl::elm::NamedStepTrackBar* autoGPUVminTrackbar  = nullptr; // EOS: 3-step Off/Official/Hijack
        tsl::elm::NamedStepTrackBar* gpuVminOffsetTrackbar = nullptr;
        tsl::elm::NamedStepTrackBar* cpuGovMinTrackbar     = nullptr; // HOC: cpu_gov_min_freq
        tsl::elm::ListItem* refreshRateItem = nullptr;

        // Tracks the mV value we last wrote so refresh() doesn't clobber the
        // trackbar position with a stale file read between rapid user clicks.
        // Sentinel -999 means "not yet initialised" → first refresh always syncs.
        int m_dvfsOffsetWritten    = -999; // HOC: dvfs_offset
        int m_eosVminOffsetWritten = -999; // EOS: gpu_vmin_offset
        int m_cpuGovMinWritten     = -1;   // HOC: cpu_gov_min_freq (sentinel -1 = uninitialised)

        u8 frameCounter = 60;
        bool m_pendingGovSwap = false;
};