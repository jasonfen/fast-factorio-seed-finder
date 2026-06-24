#pragma once

#include "finder.hpp"

// Combined finder: a defensible peninsula spawn that ALSO has workable ore nearby.
//
// The two criteria have very different costs, so they are split into two stages
// to exploit the pipeline:
//
//   Stage 1 - spawn-resource gate (cheap). Uses only regular patches, which are
//             identical for twin seeds and independent of water/elevation. So it
//             runs over even seeds only, with no twin/water/elevation expansion,
//             and eliminates the large majority of seeds before any water work.
//
//   Stage 2 - peninsula water check (expensive). Runs only on the stage-1
//             survivors. Water/elevation differ between twins, so each surviving
//             even seed is expanded to its odd twin and both are scored.
//
// Final ranking is by peninsula score among seeds that already have good ore.

constexpr Finder<>::StageSettings stage1_settings{
    .check_twin_seeds = false,
    .check_water_settings = false,
    .check_elevation_types = false,

    // Pass-or-fail gate; let stage 2 do the ranking. This is just an upper bound
    // on how many survivors are carried forward (and thus a memory cap). A strong
    // gate should leave far fewer than this.
    .seed_nb_to_next_stage = 5'000'000
};

constexpr Finder<>::StageSettings stage2_settings{
    .check_twin_seeds = true,
    .check_water_settings = false,
    .check_elevation_types = false,

    .seed_nb_to_next_stage = 1'000    // Keep the top 1000 most water-surrounded seeds.
};

// No data is passed between stages, so the void seed cache is used and the
// seed-cache pointer is unused.
Finder<>::EvalResult stage1_eval(
    const MapGenSettings&, const NoisePrecompute&, NoiseCache&, uint32_t seed, void*);
Finder<>::EvalResult stage2_eval(
    const MapGenSettings&, const NoisePrecompute&, NoiseCache&, uint32_t seed, void*);
