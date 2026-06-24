#include "stages.hpp"

int main(int argc, char* argv[]) {
    // Finds maps whose spawn sits on a landmass surrounded by water (a peninsula).

    MapGenSettings settings;

    // Resources and biters don't affect water, so disable them to keep generation
    // cheap. In this order: iron, copper, coal, stone, oil.
    settings.frequencies = { 0.f, 0.f, 0.f, 0.f, 0.f };
    settings.sizes = { 0.f, 0.f, 0.f, 0.f, 0.f };
    settings.biter_frequency = 0.f;
    settings.biter_size = 0.f;

    // No data is passed between stages, so the void seed cache is used.
    Finder<> finder(settings);

    // Peninsulas only depend on water/elevation. Evaluate at default water
    // settings and the 2.0 elevation type.
    finder.set_water_scales({ 1.f });
    finder.set_water_coverages({ 1.f });
    finder.set_elevation_types({ ELEVATION_2_0 });

    finder.add_stage(stage1_eval, stage1_settings);

    return finder.run("peninsula", argc, argv);
}
