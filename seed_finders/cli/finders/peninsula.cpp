#include "finders/peninsula.hpp"
#include "finders/peninsula_detect.hpp"

#include "noise.hpp"

namespace peninsula {
namespace {

// Water/elevation differ between twin seeds, so every seed is checked. This is
// the only stage: there is no cheaper pre-filter because water only exists in
// the expensive elevation step.
constexpr Finder<>::StageSettings stage_settings{
    .check_twin_seeds = true,
    .check_water_settings = false,
    .check_elevation_types = false,
    .seed_nb_to_next_stage = 1'000
};

Finder<>::EvalResult eval(
    const MapGenSettings& settings, const NoisePrecompute& precompute, NoiseCache&, uint32_t seed, void*
) {
    Noise noise(seed, true, settings.elevation_type == ELEVATION_2_0);
    return { .eliminate = false, .score = peninsula_detect::score(noise, settings, precompute) };
}

} // namespace

FinderEntry entry() {
    return FinderEntry{
        .name = "peninsula",
        .description = "Spawn enclosed by water (coastline only; resource settings ignored).",
        .run = [](const MapGenSettings& settings, const StandardOptions& options) {
            Finder<> finder(settings);
            finder.set_water_scales({ settings.water_scale });
            finder.set_water_coverages({ settings.water_coverage });
            finder.set_elevation_types({ settings.elevation_type });
            finder.add_stage(eval, stage_settings);
            return finder.execute(options);
        }
    };
}

} // namespace peninsula
