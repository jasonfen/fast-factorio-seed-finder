#include "stages.hpp"

int main(int argc, char* argv[]) {
    MapGenSettings settings;
    settings.frequencies = { 6.f, 6.f, 6.f, 6.f };
    settings.sizes = { 6.f, 6.f, 6.f, 6.f };
    settings.water_coverage = 1.f;
    settings.water_scale = 1.f;

    Finder finder(settings);

    finder.add_stage_with_cache<Stage1Cache>(stage1_eval, stage1_settings);

    return finder.run("mmp_seed_finder", argc, argv);
}