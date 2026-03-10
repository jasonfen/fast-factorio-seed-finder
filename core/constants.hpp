#pragma once

#include "utils.hpp"
#include <cstdint>
#include <array>
#include "math.hpp"

static constexpr uint32_t SEED_MAX = 4294967295;

static constexpr uint8_t QUICK_MULTIOCTAVE_MAX_SEED_OFFSET = 5;

static constexpr uint8_t MAKE_0_12LIKE_LAKES_PERSISTENCE_SEED1 = 1;
static constexpr uint8_t MAKE_0_12LIKE_LAKES_BIAS_SEED1A = 1;
static constexpr uint8_t MAKE_0_12LIKE_LAKES_BIAS_SEED1B = 2;
static constexpr std::array<float, NB_ELEVATION_TYPE> MAKE_0_12LIKE_LAKES_BIAS = { 0.f, 20.f, -1000.f };
static constexpr uint32_t TERRAIN_OCTAVES = 8;
static constexpr uint32_t MAKE_0_12LIKE_LAKES_PERSISTENCE_OCTAVES = TERRAIN_OCTAVES - 2;
static constexpr float MAKE_0_12LIKE_LAKES_PERSISTENCE_PERSISTENCE = 0.7f;
static const float MAKE_0_12LIKE_LAKES_PERSISTENCE_OUTPUT_SCALE =
    (1.f - MAKE_0_12LIKE_LAKES_PERSISTENCE_PERSISTENCE) /
    std::exp2f(MAKE_0_12LIKE_LAKES_PERSISTENCE_OCTAVES) /
    (1.f - std::powf(MAKE_0_12LIKE_LAKES_PERSISTENCE_PERSISTENCE, MAKE_0_12LIKE_LAKES_PERSISTENCE_OCTAVES)) *
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

static const float STARTER_LAKE_INPUT_SCALE2 =
    STARTER_LAKE_INPUT_SCALE * std::powf(STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER, (STARTER_LAKE_OCTAVES - 1));
static const float STARTER_LAKE_OUTPUT_SCALE2 = STARTER_LAKE_OUTPUT_SCALE * Math::exp2f(STARTER_LAKE_OCTAVES - 1);
static constexpr float STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER2 = 1 / STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER;

static constexpr float ELEVATION_MAGNITUDE = 20.f;
static constexpr float WLC_AMPLITUDE = 2.f;

enum class Seed0CustomOffsets {
    NAUVIS_HILLS,
    NAUVIS_HILLS_CLIFF_LEVEL,
    NAUVIS_BRIDGE_BILLOWS,
    NAUVIS_PERSISTANCE,
    NAUVIS_DETAIL,
    NAUVIS_MACRO_1,
    NAUVIS_MACRO_2,
    NB
};

constexpr Seed0CustomOffsets& operator++(Seed0CustomOffsets& type) {
    assert(type < Seed0CustomOffsets::NB);
    return type = (Seed0CustomOffsets)(1 + ((int)type));
}

constexpr Seed0CustomOffsets operator++(Seed0CustomOffsets& type, int) {
    Seed0CustomOffsets tmp(type);
    ++type;
    return tmp;
}

static constexpr std::array<uint32_t, (size_t)Seed0CustomOffsets::NB> SEED0_CUSTOM_OFFSETS = {
    900, 99584, 700, 500, 600, 1000, 1100
};

static const float NAUVIS_PERSISTANCE_OUTPUT_SCALE =
    (1.f - 0.7f) / std::exp2f(5.f) / (1.f - std::powf(0.7f, 5.f)) * 0.5f;
static const float STARTING_LAKE_NOISE_INPUT_SCALE = 1.f/8 * std::powf(0.5f, 4.f - 1);
static const float STARTING_LAKE_NOISE_OUTPUT_SCALE = 0.8f * std::exp2f(4.f - 1);

static constexpr std::array<int32_t, NB_PATCH_TYPE> REGION_SIZES{
    240, 1024, 512
};

static constexpr std::array<int32_t, NB_PATCH_TYPE> SPANS{
    4, 6, 1
};

static constexpr std::array<int32_t, NB_PATCH_TYPE> NB_CANDIDATES{
    32 * SPANS[STARTER] + 3,
    22 * SPANS[REGULAR] + 1,
    100
};

static constexpr std::array<float, NB_PATCH_TYPE> SUGGESTED_DISTANCES{
    32.f, 45.254833995939045f, 0.f
};

static constexpr std::array<int32_t, NB_PATCH_TYPE> CHUNK_SIZES{
    32, 46, 0
};

static constexpr std::array<int32_t, NB_PATCH_TYPE> CHUNK_COUNTS{
    1 + REGION_SIZES[STARTER] / CHUNK_SIZES[STARTER],
    1 + REGION_SIZES[REGULAR] / CHUNK_SIZES[REGULAR],
    0
};

static constexpr std::array<uint8_t, NB_PATCH_TYPE> SEEDS1{
    101, 100, 123
};

static constexpr std::array<int32_t, NB_RESOURCE_TYPE> REGULAR_NB_SPOTS{
    22, 22, 21, 21, 21
};
static constexpr uint32_t REGULAR_MAX_NB_SPOTS = 22;
static constexpr uint32_t STARTER_NB_SPOTS = 32;
static constexpr uint32_t MAX_NB_SPOTS = 32;

static constexpr std::array<float, NB_RESOURCE_TYPE> BASE_DENSITIES{
    10.f, 8.f, 8.f, 4.f, 8.2f
};

static constexpr std::array<std::array<float, NB_RESOURCE_TYPE>, NB_PATCH_TYPE> RQ_FACTORS{
    std::array<float, NB_RESOURCE_TYPE>{ 1.5f/7.f, 1.2f/7.f, 1.1f/7.f, 1.1f/7.f, 0.f },
    std::array<float, NB_RESOURCE_TYPE>{ 0.11f, 0.11f, 0.10f, 0.10f, 0.10f }
};

static constexpr std::array<int, NB_RESOURCE_TYPE> OFFSETS{
    0, 1, 2, 3, 4
};

static constexpr std::array<float, NB_RESOURCE_TYPE> BASES_SPOTS_PER_KM2{
    2.5f, 2.5f, 2.5f, 2.5f, 1.8f
};

static constexpr float REGULAR_PATCH_FADE_IN_DISTANCE = 300.f;
static constexpr float STARTING_RESOURCE_PLACEMENT_RADIUS = 120.f;
static constexpr float DOUBLE_DENSITY_DISTANCE = 1300.f;
static constexpr float RANDOM_SPOT_SIZE_MINIMUM = 0.25f;
static constexpr float RANDOM_SPOT_SIZE_MAXIMUM = 2.f;
static constexpr float STARTING_PATCHES_SPLIT = 0.5f;