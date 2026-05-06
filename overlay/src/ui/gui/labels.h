/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 * Ported to sys-clk-develop_overlay for HOC governing display.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#pragma once
#include <map>
#include <cstdint>
#include <string>

// CPU frequency label maps (used when show_governing = 1 in HOC mode)
extern std::map<uint32_t, std::string> cpu_freq_label_m;       // Mariko, no extra UV
extern std::map<uint32_t, std::string> cpu_freq_label_m_uv;    // Mariko, CPU UV High
extern std::map<uint32_t, std::string> cpu_freq_label_e;       // Erista, no UV
extern std::map<uint32_t, std::string> cpu_freq_label_e_uv;    // Erista, CPU UV

// GPU frequency label maps
extern std::map<uint32_t, std::string> gpu_freq_label_m;       // Mariko HiOPT (UV=0)
extern std::map<uint32_t, std::string> gpu_freq_label_m_slt;   // Mariko HiOPT-15 (UV=1)
extern std::map<uint32_t, std::string> gpu_freq_label_m_hiopt; // Mariko High UV (UV=2)
extern std::map<uint32_t, std::string> gpu_freq_label_e;       // Erista HiOPT (UV=0)
extern std::map<uint32_t, std::string> gpu_freq_label_e_uv;    // Erista UV (UV=1/2)

// Indexed by UV level (0/1/2) — use when you know the UV setting
extern std::map<uint32_t, std::string>* marikoGpuLabels[3];
extern std::map<uint32_t, std::string>* eristaGpuLabels[3];
