#include "stages.hpp"

int main(int argc, char* argv[]) {
    // Example seed finder that tries to find maps with as little ore as possible.

    MapGenSettings settings;

    // Defines what frequencies and sizes should be used for each patch type.
    // In this order: iron, copper, coal, stone, oil.
    // For this seed finder we don't look at oil.
    settings.frequencies = { 6.f, 6.f, 6.f, 6.f, 0.f };
    settings.sizes = { 6.f, 6.f, 6.f, 6.f, 0.f };

    // Create a finder with a seed cache, see stages.cpp for more details.
    Finder<SeedCache> finder(settings);

    // Defines what water settings should be checked. 
    // Here we set them to each possible values with the in-game map settings slider.
    finder.set_water_scales({ 1.f/6, 0.25f, 1.f/3, 0.5f, 0.75f, 1.f, 4.f/3, 1.5f, 2.f, 3.f, 4.f, 6.f });
    finder.set_water_coverages({ 1.f/6, 0.25f, 1.f/3, 0.5f, 0.75f, 1.f, 4.f/3, 1.5f, 2.f, 3.f, 4.f, 6.f });

    // Defines what map types should be checked, for this seed finder island map type would be cheating.
    finder.set_elevation_types({ ELEVATION_2_0, ELEVATION_1_1 });

    // Add stages.
    finder.add_stage(stage1_eval, stage1_settings);
    finder.add_stage_with_cache<Stage2Cache>(stage2_eval, stage2_settings); // This stage has a stage cache, see stages.cpp for more details.
    finder.add_stage(stage3_eval, stage3_settings);

    return finder.run("example", argc, argv);
}