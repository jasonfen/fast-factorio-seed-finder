#pragma once

#include "finder.hpp"

constexpr Finder::StageSettings stage1_settings{
    .check_twin_seeds = false,
    .seed_nb_to_next_stage = 1000
};

struct Stage1Cache {
    using Masks = std::array<std::pair<uint64_t, uint64_t>, REGULAR_MAX_NB_SPOTS*REGULAR_MAX_NB_SPOTS*REGULAR_MAX_NB_SPOTS*REGULAR_MAX_NB_SPOTS>;

    Masks masks; 
};

Finder::EvalResult stage1_eval(const MapGenSettings&, const NoisePrecompute&, NoiseCache&, Finder::SeedScorePair pair, Stage1Cache&);