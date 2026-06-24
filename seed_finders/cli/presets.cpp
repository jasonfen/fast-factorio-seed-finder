#include "presets.hpp"

namespace {

MapGenSettings normal_settings() {
    // Default map settings. In this order: iron, copper, coal, stone, oil.
    MapGenSettings s;
    s.frequencies = { 1.f, 1.f, 1.f, 1.f, 1.f };
    s.sizes       = { 1.f, 1.f, 1.f, 1.f, 1.f };
    s.richness    = { 1.f, 1.f, 1.f, 1.f, 1.f };
    s.water_scale = 1.f;
    s.water_coverage = 1.f;
    s.elevation_type = ELEVATION_2_0;
    return s;
}

MapGenSettings railworld_settings() {
    // !!! VERIFY these against your game's "Rail World" preset !!!
    // Placeholders in the right direction (sparse but large and rich ore,
    // default water). Export the Rail World map-exchange string in-game and copy
    // the per-resource frequency/size/richness control values here (1.0 == the
    // "normal" slider position). Water is default, so coastlines match NORMAL.
    MapGenSettings s;
    s.frequencies = { 1.f/6, 1.f/6, 1.f/6, 1.f/6, 1.f/6 };
    s.sizes       = { 3.f,   3.f,   3.f,   3.f,   3.f   };
    s.richness    = { 6.f,   6.f,   6.f,   6.f,   6.f   };
    s.water_scale = 1.f;
    s.water_coverage = 1.f;
    s.elevation_type = ELEVATION_2_0;
    return s;
}

} // namespace

const std::vector<Preset>& presets() {
    static const std::vector<Preset> table = {
        { "normal",    "Default map settings.",                   normal_settings() },
        { "railworld", "Rail World preset (PLACEHOLDER values).", railworld_settings() },
    };
    return table;
}

std::optional<MapGenSettings> preset_settings(const std::string& name) {
    for (const Preset& p : presets()) {
        if (p.name == name) return p.settings;
    }
    return std::nullopt;
}
