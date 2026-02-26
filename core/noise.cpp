#include "noise.hpp"
#include "gradients.hpp"
#include <numeric>
#include <cmath>

std::pair<float, float> starter_lake_position(uint32_t seed) {
    Random random(seed);

    constexpr float d = 75.f;
    constexpr float factor = M_PI * std::exp2(-31);
    float angle = factor * random.random();

    return std::make_pair(d*std::cos(angle), d*std::sin(angle));
}

Noise::Noise(uint32_t seed0, bool quick_multioctave_capable) {
    Random random(seed0);

    int max = quick_multioctave_capable ? 1 : QUICK_MULTIOCTAVE_MAX_SEED_OFFSET;
    for (int i = 0; i < max; i++) {
        auto& p = _permutations[i];

        std::iota(p.p1.begin(), p.p1.end(), 0);
        std::iota(p.p2.begin(), p.p2.end(), 0);
        std::iota(p.p3.begin(), p.p3.end(), 0);
        p.grad = default_gradients();
    
        random.shuffle(p.p1);
        random.shuffle(p.p2);
        random.shuffle(p.p3);
        random.shuffle(p.grad);
    }

    _starter_lake = starter_lake_position(seed0);
}

std::pair<float, float> Noise::_gradient(uint8_t x, uint8_t y, uint8_t p1, const Permutations& p) {
    uint8_t y_permutation = p1 ^ p.p2[y];
    uint8_t xy_permutation = y_permutation ^ p.p3[x];
    return p.grad[xy_permutation];
}

float Noise::_noise(uint8_t seed0_offset, uint8_t seed1, float x, float y, float input_scale, float output_scale, float offset_x, float offset_y) {
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

    const std::array<std::pair<float, float>, 4> points{
        std::make_pair( x_floor, y_floor ),
        std::make_pair( x_ceil, y_floor ),
        std::make_pair( x_floor, y_ceil ),
        std::make_pair( x_ceil, y_ceil )
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

float Noise::noise(uint8_t seed1, float x, float y, float input_scale, float output_scale, float offset_x, float offset_y) {
    return _noise(0, seed1, x, y, input_scale, output_scale, offset_x, offset_y);
}

float Noise::persistence_multioctave_noise(uint8_t seed1, float x, float y, float persistence, float octaves,
    float input_scale, float output_scale, float offset_x, float offset_y
) {
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
    float octave_output_scale_multiplier, float octave_seed0_shift
) {

}

float Noise::make_0_12like_lakes(const MapGenSettings& settings, const NoisePrecompute& precompute, float x, float y) {
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
        0.1 * settings.water_scale * distance +
        persistence_multioctave_noise(
            MAKE_0_12LIKE_LAKES_BIAS_SEED1B, x, y, persistence, MAKE_0_12LIKE_LAKES_OCTAVES_B,
            input_scale, MAKE_0_12LIKE_LAKES_OUTPUT_SCALE, offset_x, 0
        );
    
    return std::max(a, b);
}

float Noise::finish_elevation(const MapGenSettings& settings, const NoisePrecompute& precompute, float elevation, float x, float y) {
    const float dx = x - _starter_lake.first;
    const float dy = y - _starter_lake.second;
    const float starting_lake_distance = std::sqrt(dx*dx + dy*dy);

    const float starting_lake_noise = quick_multioctave_noise(
        STARTER_LAKE_SEED1, x, y, STARTER_LAKE_OCTAVES, STARTER_LAKE_INPUT_SCALE2,
        STARTER_LAKE_OUTPUT_SCALE2, 0.f, 0.f, STARTER_LAKE_OCTAVE_INPUT_SCALE_MULTIPLIER2,
        2.f, 1
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

float Noise::elevation_lakes(const MapGenSettings& settings, const NoisePrecompute& precompute, float x, float y) {
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

Patches starter_patches(const NoisePrecompute&, NoiseCache&, const uint32_t) {
    assert(false && "TODO");
    Patches patches;
    return patches;
}

static constexpr float regular_density_modifier(const int32_t x, const int32_t y) {
    const float d = std::sqrtf((float)x*x + (float)y*y);

    const float closeness_penalty = std::clamp((d - STARTING_RESOURCE_PLACEMENT_RADIUS) / REGULAR_PATCH_FADE_IN_DISTANCE, 0.f, 1.f);

    const float size_effective_distance_at = d - REGULAR_PATCH_FADE_IN_DISTANCE;
    const float distance_bonus = 1 + std::clamp(size_effective_distance_at / DOUBLE_DENSITY_DISTANCE, 0.f, 1.f);

    return closeness_penalty * distance_bonus;
}

static constexpr float regular_quantity(const float base_quantity, const float density, const float penalty) {
    return base_quantity * density * penalty;
}

static constexpr float regular_radius(const float rq_factor, const float quantity) {
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
        const auto nb_spots = NB_SPOTS[REGULAR][type];
        const auto offset = OFFSETS[type];

        for (int i = 0; i < nb_spots; i++) {
            auto &candidate = candidates[i*SPAN + offset];
            const float density = base_density * regular_density_modifier(candidate.x, candidate.y);

            candidate.density = density;
            total_density += density;
        }

        // In the original algorithm we sort the candidates using the favorability expression,
        // but for the regular patches it is always 1, so we can skip that sort.

        int penalties_offset = MAX_NB_SPOTS - nb_spots;
        const auto& penalties = precompute.get_penalties()[candidates[offset].x+512 + (candidates[offset].y+512)*1024];

        total_density = REGION_SIZES[REGULAR]*REGION_SIZES[REGULAR] * total_density / nb_spots;
        float total_quantity = 0.f;

        for (int i = 0; i < nb_spots; i++) {
            const auto &candidate = candidates[i*SPAN + offset];
            const float quantity = regular_quantity(base_quantity, candidate.density, penalties[i + penalties_offset]);
            const float radius = regular_radius(rq_factor, quantity);

            patches[type].insert(candidate.x, candidate.y, radius, quantity);
            total_quantity += quantity;

            if (total_quantity >= total_density) break;
        }
    }

    return patches;
}