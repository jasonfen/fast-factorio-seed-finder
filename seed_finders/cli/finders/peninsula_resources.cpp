#include "finders/peninsula_resources.hpp"
#include "finders/peninsula_detect.hpp"

#include <array>

#include "noise.hpp"
#include "algorithm.hpp"

namespace peninsula_resources {
namespace {

// --- Stage 1: spawn-resource gate (regular patches in the spawn region) ---
// Each of these ores must have a patch reaching within RESOURCE_RADIUS tiles of
// spawn at >= MIN_PATCH_RADIUS in size. Regular patches start ~250 tiles out, so
// keep RESOURCE_RADIUS <= 512 (one region around spawn).
constexpr std::array<ResourceType, 4> REQUIRED_RESOURCES = { IRON, COPPER, COAL, STONE };
constexpr int32_t RESOURCE_RADIUS = 400;
constexpr float   MIN_PATCH_RADIUS = 12.f;

// Stage 1 is cheap (regular patches only, even seeds, no twin/water/elevation
// expansion) and carries the bulk of the elimination.
constexpr Finder<>::StageSettings stage1_settings{
    .check_twin_seeds = false,
    .check_water_settings = false,
    .check_elevation_types = false,
    .seed_nb_to_next_stage = 5'000'000   // upper bound / memory cap; a strong gate leaves far fewer
};

// Stage 2 runs the peninsula water check only on the survivors. Water differs
// between twins, so each surviving even seed is expanded to its odd twin.
constexpr Finder<>::StageSettings stage2_settings{
    .check_twin_seeds = true,
    .check_water_settings = false,
    .check_elevation_types = false,
    .seed_nb_to_next_stage = 1'000
};

Finder<>::EvalResult stage1(
    const MapGenSettings&, const NoisePrecompute& precompute, NoiseCache& noise_cache, uint32_t seed, void*
) {
    Patches patches = regular_patches(precompute, noise_cache, seed, { 0, 0 });

    float ore_area = 0.f;
    for (ResourceType type : REQUIRED_RESOURCES) {
        bool found = false;
        for (const Patch& p : patches[type]) {
            if (p.radius < MIN_PATCH_RADIUS) continue;

            const float dx = (float)p.pos.x, dy = (float)p.pos.y;
            const float reach = (float)RESOURCE_RADIUS + p.radius;
            if (dx * dx + dy * dy <= reach * reach) {
                found = true;
                ore_area += p.radius * p.radius;
            }
        }
        if (!found) return { .eliminate = true, .score = 0.f };
    }

    return { .eliminate = false, .score = ore_area };
}

Finder<>::EvalResult stage2(
    const MapGenSettings& settings, const NoisePrecompute& precompute, NoiseCache&, uint32_t seed, void*
) {
    Noise noise(seed, true, settings.elevation_type == ELEVATION_2_0);
    return { .eliminate = false, .score = peninsula_detect::score(noise, settings, precompute) };
}

} // namespace

FinderEntry entry() {
    return FinderEntry{
        .name = "peninsula_resources",
        .description = "Peninsula spawn that also has workable ore nearby (resource gate -> water).",
        .run = [](const MapGenSettings& settings, const StandardOptions& options) {
            Finder<> finder(settings);
            finder.set_water_scales({ settings.water_scale });
            finder.set_water_coverages({ settings.water_coverage });
            finder.set_elevation_types({ settings.elevation_type });
            finder.add_stage(stage1, stage1_settings);
            finder.add_stage(stage2, stage2_settings);
            return finder.execute(options);
        }
    };
}

} // namespace peninsula_resources
