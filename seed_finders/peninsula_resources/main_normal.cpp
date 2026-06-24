#include "stages.hpp"

int main(int argc, char* argv[]) {
    // NORMAL (default) map settings: peninsula spawn with workable ore nearby.

    MapGenSettings settings;

    // Default resource generation. In this order: iron, copper, coal, stone, oil.
    settings.frequencies = { 1.f, 1.f, 1.f, 1.f, 1.f };
    settings.sizes       = { 1.f, 1.f, 1.f, 1.f, 1.f };
    settings.richness    = { 1.f, 1.f, 1.f, 1.f, 1.f };

    // Biters don't affect regular patches or water, so they're irrelevant here.

    Finder<> finder(settings);

    // Default water and the 2.0 elevation type. (Coastlines depend only on these
    // plus the seed, so this run is independent of the resource settings above.)
    finder.set_water_scales({ 1.f });
    finder.set_water_coverages({ 1.f });
    finder.set_elevation_types({ ELEVATION_2_0 });

    finder.add_stage(stage1_eval, stage1_settings);
    finder.add_stage(stage2_eval, stage2_settings);

    return finder.run("peninsula_resources_normal", argc, argv);
}
