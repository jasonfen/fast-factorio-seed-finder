#pragma once

#include "utils.hpp"
#include <cstdint>
#include <array>

static constexpr uint32_t SEED_MAX = 4294967295;

static constexpr uint8_t QUICK_MULTIOCTAVE_MAX_SEED_OFFSET = 5;

static constexpr uint8_t MAKE_0_12LIKE_LAKES_PERSISTENCE_SEED1 = 1;
static constexpr uint8_t MAKE_0_12LIKE_LAKES_BIAS_SEED1A = 1;
static constexpr uint8_t MAKE_0_12LIKE_LAKES_BIAS_SEED1B = 2;
static constexpr float MAKE_0_12LIKE_LAKES_BIAS = 20.f;
static constexpr uint32_t TERRAIN_OCTAVES = 8;
static constexpr uint32_t MAKE_0_12LIKE_LAKES_PERSISTENCE_OCTAVES = TERRAIN_OCTAVES - 2;
static constexpr float MAKE_0_12LIKE_LAKES_PERSISTENCE_PERSISTENCE = 0.7f;
static constexpr float MAKE_0_12LIKE_LAKES_PERSISTENCE_OUTPUT_SCALE =
    (1.f - MAKE_0_12LIKE_LAKES_PERSISTENCE_PERSISTENCE) /
    std::exp2(MAKE_0_12LIKE_LAKES_PERSISTENCE_OCTAVES) /
    (1.f - std::pow(MAKE_0_12LIKE_LAKES_PERSISTENCE_PERSISTENCE, MAKE_0_12LIKE_LAKES_PERSISTENCE_OCTAVES)) *
    0.5f;
static constexpr uint32_t MAKE_0_12LIKE_LAKES_OCTAVES_A = TERRAIN_OCTAVES;
static constexpr uint32_t MAKE_0_12LIKE_LAKES_OCTAVES_B = 6;
static constexpr float MAKE_0_12LIKE_LAKES_OUTPUT_SCALE = 0.125f;

static constexpr uint8_t FINISH_ELEVATION_NOISE_SEED1 = 123;

static constexpr uint8_t STARTER_LAKE_SEED1 = 14;
static constexpr float STARTER_LAKE_INPUT_SCALE = 1.f/8;
static constexpr float STARTER_LAKE_OUTPUT_SCALE = 1.f;
static constexpr int STARTER_LAKE_OCTAVES = 5;
static constexpr float STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER = 0.5f;
static constexpr float STARTER_LAKE_PERSISTENCE = 0.75f;

static constexpr float STARTER_LAKE_INPUT_SCALE2 =
    STARTER_LAKE_INPUT_SCALE * std::pow(STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER, (STARTER_LAKE_OCTAVES - 1));
static constexpr float STARTER_LAKE_OUTPUT_SCALE2 = STARTER_LAKE_OUTPUT_SCALE * std::exp2(STARTER_LAKE_OCTAVES - 1);
static constexpr float STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER2 = 1 / STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER;


static constexpr std::array<int32_t, NB_PATCH_TYPE> REGION_SIZES{
    240, 1024
};
static constexpr int32_t MAX_REGION_SIZE = 1024;

static constexpr int32_t SPAN = 6.f;

static constexpr std::array<int32_t, NB_PATCH_TYPE> NB_CANDIDATES{
    32 * SPAN + 3, 22 * SPAN + 1
};
static constexpr int32_t MAX_NB_CANDIDATES = std::max(NB_CANDIDATES[STARTER], NB_CANDIDATES[REGULAR]);

static constexpr std::array<float, NB_PATCH_TYPE> SUGGESTED_DISTANCES{
    32.f, 45.254833995939045f
};

static constexpr std::array<int32_t, NB_PATCH_TYPE> CHUNK_SIZES{
    (int32_t)std::ceil(SUGGESTED_DISTANCES[STARTER]),
    (int32_t)std::ceil(SUGGESTED_DISTANCES[REGULAR]),
};

static constexpr std::array<int32_t, NB_PATCH_TYPE> CHUNK_COUNTS{
    1 + REGION_SIZES[STARTER] / CHUNK_SIZES[STARTER],
    1 + REGION_SIZES[REGULAR] / CHUNK_SIZES[REGULAR]
};

static constexpr std::array<uint8_t, NB_PATCH_TYPE> SEEDS1{
    101, 100
};

static constexpr std::array<std::array<int32_t, NB_RESOURCE_TYPE>, NB_PATCH_TYPE> NB_SPOTS{
    std::array<int32_t, NB_RESOURCE_TYPE>{ 32, 32, 32, 32 },
    std::array<int32_t, NB_RESOURCE_TYPE>{ 22, 22, 21, 21 }
};
constexpr int MAX_NB_SPOTS = 32;

static constexpr std::array<float, NB_RESOURCE_TYPE> BASE_DENSITIES{
    10, 8, 8, 4
};

static constexpr std::array<std::array<float, NB_RESOURCE_TYPE>, NB_PATCH_TYPE> RQ_FACTORS{
    std::array<float, NB_RESOURCE_TYPE>{ 1.5f/7.f, 1.2f/7.f, 1.1f/7.f, 1.1f/7.f },
    std::array<float, NB_RESOURCE_TYPE>{ 0.11f, 0.11f, 0.10f, 0.10f }
};

static constexpr std::array<int, NB_RESOURCE_TYPE> OFFSETS{
    0, 1, 2, 3
};
static constexpr float REGULAR_PATCH_FADE_IN_DISTANCE = 300.f;
static constexpr float STARTING_RESOURCE_PLACEMENT_RADIUS = 120.f;
static constexpr float BASE_SPOTS_PER_KM2 = 2.5f;
static constexpr float DOUBLE_DENSITY_DISTANCE = 1300.f;
static constexpr float RANDOM_SPOT_SIZE_MINIMUM = 0.25f;
static constexpr float RANDOM_SPOT_SIZE_MAXIMUM = 2.f;