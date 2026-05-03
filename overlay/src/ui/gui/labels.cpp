/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 * Ported to sys-clk-develop_overlay for HOC governing display.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include "labels.h"

// ── CPU label maps ─────────────────────────────────────────────────────────

std::map<uint32_t, std::string> cpu_freq_label_m = {
    {612000000,  "Sleep Mode"},
    {1020000000, "Stock"},
    {1224000000, "Dev OC"},
    {1785000000, "Boost Mode"},
    {1963000000, "Safe Max"},
    {2397000000, "Unsafe Max"},
    {2703000000, "Absolute Max"},
};

std::map<uint32_t, std::string> cpu_freq_label_m_uv = {
    {612000000,  "Sleep Mode"},
    {1020000000, "Stock"},
    {1224000000, "Dev OC"},
    {1785000000, "Boost Mode"},
    {2397000000, "Safe Max"},
    {2499000000, "Unsafe Max"},
    {2703000000, "Absolute Max"},
};

std::map<uint32_t, std::string> cpu_freq_label_e = {
    {612000000,  "Sleep Mode"},
    {1020000000, "Stock"},
    {1224000000, "Dev OC"},
    {1785000000, "Safe Max"},
    {2091000000, "Unsafe Max"},
    {2397000000, "Absolute Max"},
};

std::map<uint32_t, std::string> cpu_freq_label_e_uv = {
    {612000000,  "Sleep Mode"},
    {1020000000, "Stock"},
    {1224000000, "Dev OC"},
    {1785000000, "Boost Mode"},
    {2091000000, "Safe Max"},
    {2193000000, "Unsafe Max"},
    {2397000000, "Absolute Max"},
};

// ── GPU label maps ─────────────────────────────────────────────────────────

std::map<uint32_t, std::string> gpu_freq_label_e = {
    {76800000,   "Boost Mode"},
    {307200000,  "Handheld"},
    {345600000,  "Handheld"},
    {384000000,  "Handheld"},
    {422400000,  "Handheld"},
    {460800000,  "Handheld Safe Max"},
    {768000000,  "Docked"},
    {921600000,  "Safe Max"},
    {960000000,  "Unsafe Max"},
    {1075200000, "Absolute Max"},
};

std::map<uint32_t, std::string> gpu_freq_label_e_uv = {
    {76800000,   "Boost Mode"},
    {307200000,  "Handheld"},
    {345600000,  "Handheld"},
    {384000000,  "Handheld"},
    {422400000,  "Handheld"},
    {460800000,  "Handheld Safe Max"},
    {768000000,  "Docked"},
    {960000000,  "Safe Max"},
    {1075200000, "Absolute Max"},
};

std::map<uint32_t, std::string> gpu_freq_label_m = {
    {76800000,   "Boost Mode"},
    {307200000,  "Handheld"},
    {384000000,  "Handheld"},
    {460800000,  "Handheld"},
    {614400000,  "Handheld Safe Max"},
    {768000000,  "Docked"},
    {1075200000, "Safe Max"},
    {1305600000, "Unsafe Max"},
    {1536000000, "Absolute Max"},
};

std::map<uint32_t, std::string> gpu_freq_label_m_slt = {
    {76800000,   "Boost Mode"},
    {307200000,  "Handheld"},
    {384000000,  "Handheld"},
    {460800000,  "Handheld"},
    {614400000,  "Handheld Safe Max"},
    {768000000,  "Docked"},
    {1152200000, "Safe Max"},
    {1305600000, "Unsafe Max"},
    {1536000000, "Absolute Max"},
};

std::map<uint32_t, std::string> gpu_freq_label_m_hiopt = {
    {76800000,   "Boost Mode"},
    {307200000,  "Handheld"},
    {384000000,  "Handheld"},
    {460800000,  "Handheld"},
    {614400000,  "Handheld Safe Max"},
    {768000000,  "Docked"},
    {1228800000, "Safe Max"},
    {1305600000, "Unsafe Max"},
    {1536000000, "Absolute Max"},
};

// ── UV-indexed pointers ────────────────────────────────────────────────────

std::map<uint32_t, std::string>* marikoGpuLabels[3] {
    &gpu_freq_label_m,
    &gpu_freq_label_m_slt,
    &gpu_freq_label_m_hiopt,
};

std::map<uint32_t, std::string>* eristaGpuLabels[3] {
    &gpu_freq_label_e,
    &gpu_freq_label_e_uv,
    &gpu_freq_label_e_uv,
};
