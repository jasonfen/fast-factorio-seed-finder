#pragma once

#include <optional>
#include <string>
#include <vector>

#include "noise.hpp"   // MapGenSettings, ElevationType

// A named map-settings preset (a "mode"). Only the map-generation settings
// differ between presets; the goal/criteria are chosen separately with --finder.
struct Preset {
    std::string name;
    std::string description;
    MapGenSettings settings;
};

const std::vector<Preset>& presets();

// Returns the settings for the named preset, or nullopt if unknown.
std::optional<MapGenSettings> preset_settings(const std::string& name);
