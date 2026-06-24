#include "stages.hpp"

int main(int argc, char* argv[]) {
    // RAIL WORLD map settings: peninsula spawn with workable ore nearby.
    //
    // !!! VERIFY THESE VALUES against your game's "Rail World" preset !!!
    // The numbers below are placeholders in the right direction (sparse but large
    // and rich ore). To get exact values, open the Rail World preset in-game,
    // export the map-exchange string, and copy the per-resource frequency / size /
    // richness control values here (1.0 == the "normal" slider position).
    //
    // These only affect the stage-1 resource gate (regular patches). Water and
    // thus the peninsula geometry are unchanged: Rail World uses default water,
    // so its coastlines are identical to NORMAL for the same seed.

    MapGenSettings settings;

    // In this order: iron, copper, coal, stone, oil.
    settings.frequencies = { 1.f/6, 1.f/6, 1.f/6, 1.f/6, 1.f/6 };  // few, far-apart patches
    settings.sizes       = { 3.f,   3.f,   3.f,   3.f,   3.f   };  // large patches
    settings.richness    = { 6.f,   6.f,   6.f,   6.f,   6.f   };  // very rich

    Finder<> finder(settings);

    finder.set_water_scales({ 1.f });
    finder.set_water_coverages({ 1.f });
    finder.set_elevation_types({ ELEVATION_2_0 });

    finder.add_stage(stage1_eval, stage1_settings);
    finder.add_stage(stage2_eval, stage2_settings);

    return finder.run("peninsula_resources_railworld", argc, argv);
}
