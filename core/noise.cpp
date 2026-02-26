#define _USE_MATH_DEFINES
#include "noise.hpp"
#include "gradients.hpp"
#include <numeric>
#include <cmath>
#include <print>

NoisePrecompute::NoisePrecompute(const MapGenSettings& settings) {
    _starter_penalties = new std::array<float, STARTER_NB_SPOTS>[REGION_SIZES[STARTER]*REGION_SIZES[STARTER]];
    _regular_penalties = new std::array<float, REGULAR_MAX_NB_SPOTS>[REGION_SIZES[REGULAR]*REGION_SIZES[REGULAR]];

    for (int32_t i = 0; i < REGION_SIZES[STARTER]; i++) {
        for (int32_t j = 0; j < REGION_SIZES[STARTER]; j++) {
            int32_t x = i - (REGION_SIZES[STARTER] / 2);
            int32_t y = j - (REGION_SIZES[STARTER] / 2);

            Random::random_penalty_at<STARTER_NB_SPOTS>(
                _starter_penalties[i + j*REGION_SIZES[STARTER]], x, y, 0.5f, 1
            );
        }
    }

    for (int32_t i = 0; i < REGION_SIZES[REGULAR]; i++) {
        for (int32_t j = 0; j < REGION_SIZES[REGULAR]; j++) {
            int32_t x = i - (REGION_SIZES[REGULAR] / 2);
            int32_t y = j - (REGION_SIZES[REGULAR] / 2);

            Random::random_penalty_between<REGULAR_MAX_NB_SPOTS>(
                _regular_penalties[i + j*REGION_SIZES[REGULAR]], x, y, RANDOM_SPOT_SIZE_MINIMUM, RANDOM_SPOT_SIZE_MAXIMUM, 1
            );
        }
    }

    for (ResourceType type = IRON; type < NB_RESOURCE_TYPE; type++) {
        /*
        size_effective_distance_at = if(has_starting_area_placement == -1, distance, distance - regular_patch_fade_in_distance)

        regular_density_at(distance) = "base_density * frequency_multiplier * size_multiplier * \z
                        if(has_starting_area_placement == -1, 1, clamp((distance - starting_resource_placement_radius) / regular_patch_fade_in_distance, 0, 1)) * \z
                        (1 + clamp(size_effective_distance_at(distance) / double_density_distance, 0, 1))"

        regular_spot_quantity_base_at = "1000000 / base_spots_per_km2 / frequency_multiplier * regular_density_at(distance)"

        regular_spot_quantity_expression = "random_penalty_between(random_spot_size_minimum, random_spot_size_maximum, 1) * \z
                                            regular_spot_quantity_base_at(distance)"
        */
        _base_regular_densities[type] = BASE_DENSITIES[type] * settings.frequencies[type] * settings.sizes[type];
        _base_regular_quantities[type] = 1000000.f / BASE_SPOTS_PER_KM2 / settings.frequencies[type];

        /*
        density_expression = starting_amount / (pi * starting_resource_placement_radius * starting_resource_placement_radius) * \z
            starting_modulation,\z

        spot_quantity_expression = starting_area_spot_quantity,\z

        spot_radius_expression = starting_rq_factor * starting_area_spot_quantity ^ (1/3),\z

        spot_favorability_expression = clamp((elevation_lakes - 1) / 10, 0, 1) * starting_modulation * 2 - \z
            distance / starting_resource_placement_radius + random_penalty_at(0.5, 1),\z

        starting_amount = "20000 * base_density * (frequency_multiplier + 1) * size_multiplier",

        starting_area_spot_quantity = "starting_amount / starting_patches_split / frequency_multiplier",

        starting_modulation = "starting_resource_placement_radius > distance",
        */
        const float starting_amount = 20000.f * BASE_DENSITIES[type] * (settings.frequencies[type] + 1.f) * settings.sizes[type];
        const float density_expression = starting_amount /
            ((float)M_PI * STARTING_RESOURCE_PLACEMENT_RADIUS*STARTING_RESOURCE_PLACEMENT_RADIUS);
        _starter_base_densities[type] = REGION_SIZES[STARTER]*REGION_SIZES[STARTER] * density_expression / STARTER_NB_SPOTS;

        _starter_quantities[type] = starting_amount / STARTING_PATCHES_SPLIT / settings.frequencies[type];
        _starter_radii[type] = RQ_FACTORS[STARTER][type] * std::cbrtf(_starter_quantities[type]);
    }

    _water_level = 10 * std::log2(settings.water_coverage);
    _make_0_12like_lakes_input_scale = settings.water_scale / 2;
    _make_0_12like_lakes_offset_x = 10000.f / settings.water_scale;

}

std::pair<float, float> starter_lake_position(uint32_t seed) {
    Random random(seed);

    constexpr float d = 75.f;
    const float factor = (float)M_PI * std::exp2f(-31);
    float angle = factor * random.random();

    return std::make_pair(d*std::cos(angle), d*std::sin(angle));
}

std::pair<float, float> Noise::_gradient(uint8_t x, uint8_t y, uint8_t p1, const Permutations& p) const {
    uint8_t y_permutation = p1 ^ p.p2[y];
    uint8_t xy_permutation = y_permutation ^ p.p3[x];
    return p.grad[xy_permutation];
}

Noise::Noise(uint32_t seed0, bool quick_multioctave_precompute) {
    _quick_multioctave_precompute = quick_multioctave_precompute;

    const int max = quick_multioctave_precompute ? QUICK_MULTIOCTAVE_MAX_SEED_OFFSET : 1;
    for (int i = 0; i < max; i++) {
        Random random(seed0 + i);
        auto& p = _permutations[i];

        std::iota(p.p1.begin(), p.p1.end(), (uint8_t)0);
        std::iota(p.p2.begin(), p.p2.end(), (uint8_t)0);
        std::iota(p.p3.begin(), p.p3.end(), (uint8_t)0);
        p.grad = default_gradients();
    
        random.shuffle(p.p1);
        random.shuffle(p.p2);
        random.shuffle(p.p3);
        random.shuffle(p.grad);
    }

    _starter_lake = starter_lake_position(seed0);
}

float Noise::_noise_internal(
    uint32_t seed0_offset, uint8_t seed1, float x, float y, float input_scale, float output_scale, float offset_x, float offset_y
) const {
    const auto& p = _permutations[seed0_offset];

    const uint8_t p1 = p.p1[seed1];
    
    const float x_scaled = (x + offset_x) * input_scale;
    const float y_scaled = (y + offset_y) * input_scale;

    const float x_floor = std::floorf(x_scaled);
    const float y_floor = std::floorf(y_scaled);
    const float x_ceil = x_floor + 1.f;
    const float y_ceil = y_floor + 1.f;
    const float x_frac = x_scaled - x_floor;
    const float y_frac = y_scaled - y_floor;

    const uint8_t int_x_floor = (uint8_t)x_floor;
    const uint8_t int_y_floor = (uint8_t)y_floor;
    const uint8_t int_x_ceil = (uint8_t)x_ceil;
    const uint8_t int_y_ceil = (uint8_t)y_ceil;
    const std::array<std::pair<uint8_t, uint8_t>, 4> points{
        std::make_pair( int_x_floor, int_y_floor ),
        std::make_pair( int_x_ceil, int_y_floor ),
        std::make_pair( int_x_floor, int_y_ceil ),
        std::make_pair( int_x_ceil, int_y_ceil )
    };

    const std::array<std::pair<float, float>, 4> dcba{
        std::make_pair( 0.f, 0.f ),
        std::make_pair( 1.f, 0.f ),
        std::make_pair( 0.f, 1.f ),
        std::make_pair( 1.f, 1.f )
    };

    float sum = 0.f;

    for (int i = 0; i < 4; i++) {
        const auto gradient = _gradient(points[i].first, points[i].second, p1, p);

        const float x_frac_offset = x_frac - dcba[i].first;
        const float y_frac_offset = y_frac - dcba[i].second;

        const float d2 = 1.f - std::min(1.f, x_frac_offset*x_frac_offset + y_frac_offset*y_frac_offset);

        sum += (
            (x_frac_offset * gradient.first) +
            (y_frac_offset * gradient.second)
        ) * d2*d2*d2;
    }

    return sum * output_scale;
}

float Noise::noise(uint8_t seed1, float x, float y, float input_scale, float output_scale, float offset_x, float offset_y) const {
    return _noise_internal(0, seed1, x, y, input_scale, output_scale, offset_x, offset_y);
}

float Noise::persistence_multioctave_noise(uint8_t seed1, float x, float y, float persistence, float octaves,
    float input_scale, float output_scale, float offset_x, float offset_y
) const {
    input_scale *= 0.5f;
    output_scale *= std::powf(2.f, octaves);

    float sum = 0.f;
    for (int i = 1; i < octaves; i++) {
        sum += noise(seed1, x, y, input_scale, 1.f, offset_x, offset_y);
        sum *= persistence;

        input_scale *= 0.5f;
    }

    sum += noise(seed1, x, y, input_scale, 1.f, offset_x, offset_y);

    return sum * output_scale;
}

float Noise::quick_multioctave_noise(
    uint8_t seed1, float x, float y, uint32_t octaves, float input_scale,
    float output_scale, float offset_x, float offset_y, float octave_input_scale_multiplier,
    float octave_output_scale_multiplier, uint32_t octave_seed0_shift
) const {
    assert(_quick_multioctave_precompute);
    assert(octave_seed0_shift * octaves <= QUICK_MULTIOCTAVE_MAX_SEED_OFFSET);

    float sum = 0.f;

    for (uint32_t i = 0; i < octaves; i++) {
        sum += _noise_internal(i*octave_seed0_shift, seed1, x, y, input_scale, output_scale, offset_x, offset_y);

        input_scale *= octave_input_scale_multiplier;
        output_scale *= octave_output_scale_multiplier;
    }

    return sum;
}

/*
elevation_lakes = finish_elevation{elevation = make_0_12like_lakes{x = x,\z
                                                                   y = y,\z
                                                                   bias = 20,\z
                                                                   terrain_octaves = 8,\z
                                                                   segmentation_multiplier = segmentation_multiplier},\z
                                   segmentation_multiplier = segmentation_multiplier}

finish_elevation = min((elevation - water_level) / segmentation_multiplier,\z
                      basis_noise{x = x, y = y, seed0 = map_seed, seed1 = 123, input_scale = 1/8, output_scale = 1.5} + \z
                      starting_lake_distance / 4 - 4,\z
                      -1 + (starting_lake_distance + starting_lake_noise) / 16,\z
                      max(2, 2 + starting_lake_distance / 16 + starting_lake_noise / 2))

{
    type = "noise-expression",
    name = "water_level",
    expression = "10 * log2(control:water:size)"
},

starting_lake_distance = "distance_from_nearest_point{x = x, y = y, points = starting_lake_positions, maximum_distance = 1024}",
starting_lake_noise = "quick_multioctave_noise_persistence{x = x,\z
                                                            y = y,\z
                                                            seed0 = map_seed,\z
                                                            seed1 = 14,\z
                                                            input_scale = 1/8,\z
                                                            output_scale = 1,\z
                                                            octaves = 5,\z
                                                            octave_input_scale_multiplier = 0.5,\z
                                                            persistence = 0.75}"

  {
    type = "noise-function",
    name = "quick_multioctave_noise_persistence",
    parameters = {"x", "y", "seed0", "seed1", "input_scale", "output_scale", "octaves", "octave_input_scale_multiplier", "persistence"},
    expression = "quick_multioctave_noise{x = x,\z
                                          y = y,\z
                                          seed0 = seed0,\z
                                          seed1 = seed1,\z
                                          input_scale = input_scale * octave_input_scale_multiplier ^ (octaves - 1),\z
                                          output_scale = output_scale * 2 ^ (octaves - 1),\z
                                          octaves = octaves,\z
                                          octave_output_scale_multiplier = persistence,\z
                                          octave_input_scale_multiplier = 1 / octave_input_scale_multiplier}"
  },

  {
    type = "noise-function",
    name = "make_0_12like_lakes",
    parameters = {"x", "y", "bias", "terrain_octaves", "segmentation_multiplier"},
    expression = "max(bias + variable_persistence_multioctave_noise{x = x,\z
                                                                    y = y,\z
                                                                    seed0 = map_seed,\z
                                                                    seed1 = 1,\z
                                                                    input_scale = input_scale,\z
                                                                    output_scale = 0.125,\z
                                                                    offset_x = offset_x,\z
                                                                    octaves = terrain_octaves,\z
                                                                    persistence = persistence},\z
                      20 + water_level - 0.1 * segmentation_multiplier * distance + \z
                      variable_persistence_multioctave_noise{x = x,\z
                                                             y = y,\z
                                                             seed0 = map_seed,\z
                                                             seed1 = 2,\z
                                                             input_scale = input_scale,\z
                                                             output_scale = 0.125,\z
                                                             offset_x = offset_x,\z
                                                             octaves = 6,\z
                                                             persistence = persistence})",
    local_expressions =
    {
      input_scale = "segmentation_multiplier / 2",
      offset_x = "10000 / segmentation_multiplier",
      persistence = "clamp(amplitude_corrected_multioctave_noise{x = x,\z
                                                                 y = y,\z
                                                                 seed0 = map_seed,\z
                                                                 seed1 = 1,\z
                                                                 octaves = terrain_octaves - 2,\z
                                                                 input_scale = input_scale,\z
                                                                 offset_x = offset_x,\z
                                                                 persistence = 0.7,\z
                                                                 amplitude = 0.5} + 0.3,\z
                          0.1, 0.9)"
    }

  {
    type = "noise-function",
    name = "amplitude_corrected_multioctave_noise",
    parameters = {"x", "y", "seed0", "seed1", "input_scale", "offset_x", "octaves", "persistence", "amplitude"},
    expression = "variable_persistence_multioctave_noise{x = x,\z
                                                         y = y,\z
                                                         seed0 = seed0,\z
                                                         seed1 = seed1,\z
                                                         input_scale = input_scale,\z
                                                         output_scale = (1 - persistence) / 2 ^ octaves / (1 - persistence ^ octaves) * amplitude,\z
                                                         offset_x = offset_x,\z
                                                         octaves = octaves,\z
                                                         persistence = persistence}"
  },
*/

float Noise::make_0_12like_lakes(const MapGenSettings& settings, const NoisePrecompute& precompute, float x, float y) const {
    const float input_scale = precompute.get_make_0_12like_lakes_input_scale();
    const float offset_x = precompute.get_make_0_12like_lakes_offset_x();
    
    const float persistence = std::clamp(
        0.3f + persistence_multioctave_noise(
            MAKE_0_12LIKE_LAKES_PERSISTENCE_SEED1, x, y, MAKE_0_12LIKE_LAKES_PERSISTENCE_PERSISTENCE,
            MAKE_0_12LIKE_LAKES_PERSISTENCE_OCTAVES, input_scale, MAKE_0_12LIKE_LAKES_PERSISTENCE_OUTPUT_SCALE,
            offset_x, 0
        ),
        0.1f, 0.9f
    );

    const float distance = std::sqrt(x*x + y*y);

    const float a = MAKE_0_12LIKE_LAKES_BIAS + persistence_multioctave_noise(
        MAKE_0_12LIKE_LAKES_BIAS_SEED1A, x, y, persistence, MAKE_0_12LIKE_LAKES_OCTAVES_A,
        input_scale, MAKE_0_12LIKE_LAKES_OUTPUT_SCALE, offset_x, 0
    );

    const float b = 20.f + precompute.get_water_level() -
        0.1f * settings.water_scale * distance +
        persistence_multioctave_noise(
            MAKE_0_12LIKE_LAKES_BIAS_SEED1B, x, y, persistence, MAKE_0_12LIKE_LAKES_OCTAVES_B,
            input_scale, MAKE_0_12LIKE_LAKES_OUTPUT_SCALE, offset_x, 0
        );
    
    return std::max(a, b);
}

float Noise::finish_elevation(const MapGenSettings& settings, const NoisePrecompute& precompute, float elevation, float x, float y) const {
    const float dx = x - std::floor(_starter_lake.first);
    const float dy = y - std::floor(_starter_lake.second);
    const float starting_lake_distance = std::min(std::sqrt(dx*dx + dy*dy), 1024.f);

    const float starting_lake_noise = quick_multioctave_noise(
        STARTER_LAKE_SEED1, x, y, STARTER_LAKE_OCTAVES, STARTER_LAKE_INPUT_SCALE2,
        STARTER_LAKE_OUTPUT_SCALE2, 0.f, 0.f, STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER2,
        STARTER_LAKE_PERSISTENCE, 1
    );

    const float a = std::min(
        (elevation - precompute.get_water_level()) / settings.water_scale,
        noise(FINISH_ELEVATION_NOISE_SEED1, x, y, 1.f/8, 1.5f, 0.f, 0.f) + starting_lake_distance / 4.f - 4.f
    );

    const float b = std::min(
        -1 + (starting_lake_distance + starting_lake_noise) / 16.f,
        std::max(2.f, 2.f + starting_lake_distance / 16.f + starting_lake_noise / 2.f)
    );
    
    return std::min(a, b);
}

float Noise::elevation_lakes(const MapGenSettings& settings, const NoisePrecompute& precompute, float x, float y) const {
    float lakes = make_0_12like_lakes(settings, precompute, x, y);
    return finish_elevation(settings, precompute, lakes, x, y);
}

template<PatchType Type>
static std::array<Candidate, NB_CANDIDATES[Type]> generate_candidates(
    NoiseCache& cache, const uint32_t seed0
) {
    std::array<Candidate, NB_CANDIDATES[Type]> candidates;

    const uint32_t id = cache.get_id();
    auto& chunks = cache.get_chunks<Type>();
    constexpr auto chunk_count = CHUNK_COUNTS[Type];
    constexpr auto chunk_size = CHUNK_SIZES[Type];

    // const uint32_t seed = (region_y * 0x1ee3 + region_x * 0x1eef + seed1 * 0x1ef7 + 0x3fbe2c) ^ seed0;
    constexpr uint32_t seed_base = SEEDS1[Type] * 0x1ef7 + 0x3fbe2c;
    const uint32_t seed = seed_base ^ seed0;
    Random random(seed);

    constexpr float c_suggested_distance2 = SUGGESTED_DISTANCES[Type] * SUGGESTED_DISTANCES[Type];
    float suggested_distance2 = c_suggested_distance2;

    constexpr int32_t half_region_size = REGION_SIZES[Type] / 2;
    // const int32_t offset_x = RegionSize * region_x - half_region_size;
    // const int32_t offset_y = RegionSize * region_y - half_region_size;

    int i = 0;
    while (i < NB_CANDIDATES[Type]) {
        int32_t x = random.random() % REGION_SIZES[Type];
        int32_t y = random.random() % REGION_SIZES[Type];
        const int32_t chunk_x = x / chunk_size;
        const int32_t chunk_y = y / chunk_size;
        
        x -= half_region_size;
        y -= half_region_size;

        const int other_chunk_y_min = std::max(0, chunk_y - 1);
        const int other_chunk_x_max = std::min(chunk_count - 1, chunk_x + 1);
        const int other_chunk_y_max = std::min(chunk_count - 1, chunk_y + 1);

        bool valid = true;

        for (int other_chunk_x = std::max(0, chunk_x - 1); other_chunk_x <= other_chunk_x_max; other_chunk_x++) {
            for (int other_chunk_y = other_chunk_y_min; other_chunk_y <= other_chunk_y_max; other_chunk_y++) {
                auto& other_array = chunks[other_chunk_x + other_chunk_y * chunk_count];
                if (other_array.id != id) continue;

                for (auto& other : other_array) {
                    int32_t dx = x - other.x;
                    int32_t dy = y - other.y;
                    if (dx*dx + dy*dy < suggested_distance2) {
                        suggested_distance2 *= 0.9375f;
                        valid = false;
                        goto invalid;
                    }
                }
            }
        }
        invalid:;

        if (valid) {
            auto& candidate = candidates[i];
            candidate.x = x;
            candidate.y = y;

            auto& other_array = chunks[chunk_x + chunk_y * chunk_count];
            if (other_array.id != id) {
                other_array.clear();
                other_array.id = id;
            }
            other_array.insert(x, y);
            i++;
        }
    }

    return candidates;
}

Patches starter_patches(const MapGenSettings& settings, const NoisePrecompute& precompute, const Noise& noise, NoiseCache& cache, const uint32_t seed0) {
    Patches patches;

    auto candidates = generate_candidates<STARTER>(cache, seed0);

    for (ResourceType type = IRON; type < NB_RESOURCE_TYPE; type++) {
        const auto offset = OFFSETS[type];

        const auto& penalties = precompute.get_starter_penalties()[candidates[offset].x+120 + (candidates[offset].y+120)*240];
        std::array<Candidate*, STARTER_NB_SPOTS> candidates_ptr;

        const float base_density = precompute.get_starter_base_densities()[type];
        float total_density = 0.f;

        for (int i = 0; i < STARTER_NB_SPOTS; i++) {
            auto &candidate = candidates[i*SPANS[STARTER] + offset];
            candidates_ptr[i] = &candidate;

            const float x = (float)candidate.x;
            const float y = (float)candidate.y;
            const float distance = std::sqrtf(x*x + y*y);

            float elevation = 0.f;
            if (STARTING_RESOURCE_PLACEMENT_RADIUS > distance) {
                elevation = std::clamp((noise.elevation_lakes(settings, precompute, x, y) - 1.f) / 10.f, 0.f, 1.f) * 2.f;
                total_density += base_density;
            }
            candidate.favorability = elevation - distance / STARTING_RESOURCE_PLACEMENT_RADIUS + penalties[i];
        }

        struct CandidateCompare {
            bool operator()(const Candidate* a, const Candidate* b) const {
                return a->favorability > b->favorability;
            }
        };

        std::stable_sort(candidates_ptr.begin(), candidates_ptr.begin() + STARTER_NB_SPOTS, CandidateCompare());
        const float quantity = precompute.get_starter_quantities()[type];
        const float radius = precompute.get_starter_radii()[type];
        const float nb_patches = total_density / quantity;
        const float nb_patches_ceil = std::ceil(nb_patches);
        const int last = (int)nb_patches_ceil - 1;

        for (int i = 0; i < last; i++) {
            const auto &candidate = *candidates_ptr[i];
            patches[type].insert(candidate.x, candidate.y, radius, quantity);
        }

        const float frac = 1 - nb_patches_ceil + nb_patches;
        const float last_quantity = quantity * frac;
        // const float last_radius = radius * frac;
        const float last_radius = radius * std::cbrt(last_quantity / quantity);
        const auto &candidate = *candidates_ptr[last];
        patches[type].insert(candidate.x, candidate.y, last_radius, last_quantity);
    }

    return patches;
}

static float regular_density_modifier(const int32_t x, const int32_t y) {
    const float d = std::sqrtf((float)x*x + (float)y*y);

    const float closeness_penalty = std::clamp((d - STARTING_RESOURCE_PLACEMENT_RADIUS) / REGULAR_PATCH_FADE_IN_DISTANCE, 0.f, 1.f);

    const float size_effective_distance_at = d - REGULAR_PATCH_FADE_IN_DISTANCE;
    const float distance_bonus = 1 + std::clamp(size_effective_distance_at / DOUBLE_DENSITY_DISTANCE, 0.f, 1.f);

    return closeness_penalty * distance_bonus;
}

static float regular_quantity(const float base_quantity, const float density, const float penalty) {
    return base_quantity * density * penalty;
}

static float regular_radius(const float rq_factor, const float quantity) {
    /*
    spot_radius_expression = min(32, regular_rq_factor * regular_spot_quantity_expression ^ (1/3))
    */

    return std::min(32.f, rq_factor * std::cbrtf(quantity));
}

Patches regular_patches(const NoisePrecompute& precompute, NoiseCache& cache, const uint32_t seed0) {
    Patches patches;

    auto candidates = generate_candidates<REGULAR>(cache, seed0);

    for (ResourceType type = IRON; type < NB_RESOURCE_TYPE; type++) {
        const float base_density = precompute.get_base_regular_densities()[type];
        const float base_quantity = precompute.get_base_regular_quantities()[type];
        const float rq_factor = RQ_FACTORS[REGULAR][type];

        float total_density = 0.f;
        const auto nb_spots = REGULAR_NB_SPOTS[type];
        const auto offset = OFFSETS[type];

        for (int i = 0; i < nb_spots; i++) {
            auto &candidate = candidates[i*SPANS[REGULAR] + offset];
            const float density = base_density * regular_density_modifier(candidate.x, candidate.y);

            candidate.density = density;
            total_density += density;
        }

        int penalties_offset = REGULAR_MAX_NB_SPOTS - nb_spots;
        const auto& penalties = precompute.get_regular_penalties()[candidates[offset].x+512 + (candidates[offset].y+512)*1024];

        total_density = REGION_SIZES[REGULAR]*REGION_SIZES[REGULAR] * total_density / nb_spots;
        float total_quantity = 0.f;

        for (int i = 0; i < nb_spots; i++) {
            const auto &candidate = candidates[i*SPANS[REGULAR] + offset];
            const float quantity = regular_quantity(base_quantity, candidate.density, penalties[i + penalties_offset]);
            const float radius = regular_radius(rq_factor, quantity);

            patches[type].insert(candidate.x, candidate.y, radius, quantity);
            total_quantity += quantity;

            if (total_quantity >= total_density) break;
        }
    }

    return patches;
}