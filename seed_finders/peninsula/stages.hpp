#pragma once

#include "finder.hpp"

// This finder only inspects water/elevation, which differs between twin seeds,
// so every seed is checked (check_twin_seeds = true => no even-only skipping).
// It is a single stage: there is no cheaper pre-filter to put before it because
// water only exists in the expensive elevation step.
constexpr Finder<>::StageSettings stage1_settings{
    .check_twin_seeds = true,
    .check_water_settings = false,
    .check_elevation_types = false,

    .seed_nb_to_next_stage = 100    // Keep the top 100 most water-surrounded seeds.
};

// No SeedCache is needed (nothing is passed between stages), so the void
// specialization of Finder is used and the seed-cache pointer is unused.
Finder<>::EvalResult stage1_eval(
    const MapGenSettings&, const NoisePrecompute&, NoiseCache&, uint32_t seed, void*);
