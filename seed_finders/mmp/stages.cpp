#define _USE_MATH_DEFINES
#include "stages.hpp"

#include <print>

constexpr float MIN_IRON_AREA = (float)M_PI * 32*32;
constexpr float MIN_COPPER_AREA = (float)M_PI * 28*28;
constexpr float MIN_COAL_AREA = (float)M_PI * 20*20;
constexpr float MIN_STONE_AREA = (float)M_PI * 13*13;

constexpr float MAX_IRON_AREA = MIN_IRON_AREA * 1.25f;
constexpr float MAX_COPPER_AREA = MIN_COPPER_AREA * 1.25f;
constexpr float MAX_COAL_AREA = MIN_COAL_AREA * 1.25f;
constexpr float MAX_STONE_AREA = MIN_STONE_AREA * 1.25f;

constexpr int32_t MIN_SCORE = 3;

constexpr int32_t MAX_IRON_COPPER_DISTANCE = 100;
constexpr int32_t MAX_IRON_COAL_DISTANCE = 100;
constexpr int32_t MAX_IRON_STONE_DISTANCE = 200;
constexpr int32_t MAX_COPPER_COAL_DISTANCE = 120;
constexpr int32_t MAX_COPPER_STONE_DISTANCE = 200;
constexpr int32_t MAX_COAL_STONE_DISTANCE = 200;

constexpr int32_t MAX_PATCH_DISTANCE_FROM_SPAWN = 512;

static constexpr int32_t chebyshev_length(PositionI32 pos) {
    return std::max(std::abs(pos.x), std::abs(pos.y));
}

static constexpr int32_t chebyshev_distance(PositionI32 a, PositionI32 b) {
    return chebyshev_length(a - b);
}

// Merges close patches and removes too small ones
static void transform_patches(PatchArray& patches, float min_area, float max_area) {
    PatchArray temp;

    for (size_t i = 0; i < patches.size(); i++) {
        auto& p1 = patches[i];
        if (p1.radius == 0) continue;
        float total_area = p1.radius*p1.radius * (float)M_PI;
        PositionI32 average_position = p1.pos;
        int32_t count = 1;

        if (total_area < min_area) {
            for (size_t j = i + 1; j < patches.size() && total_area < max_area; j++) {
                auto& p2 = patches[j];
                float p2_area = p2.radius*p2.radius * (float)M_PI;
                if (p2.radius == 0 || p2_area >= min_area) continue;

                float radius_sum = p1.radius + p2.radius + 40;
                if (PositionI32::distance_2(p1.pos, p2.pos) <= radius_sum*radius_sum) {
                    total_area += p2_area;
                    average_position += p2.pos;
                    p2.radius = 0;
                    count++;
                }
            }
        }

        // Patch.radius is used as area instead
        temp.insert(average_position / count, total_area, 0);
    }

    patches.clear();

    for (const auto& p : temp) {
        if (chebyshev_length(p.pos) < MAX_PATCH_DISTANCE_FROM_SPAWN && p.radius >= min_area) {
            patches.insert(p.pos, p.radius, p.quantity);
        }
    }
}

static std::array<bool, REGULAR_MAX_NB_SPOTS*REGULAR_MAX_NB_SPOTS> make_pairs(const PatchArray& patches1, const PatchArray& patches2, int32_t max_distance) {
    std::array<bool, REGULAR_MAX_NB_SPOTS*REGULAR_MAX_NB_SPOTS> out;
    auto size1 = patches1.size(), size2 = patches2.size();

    for (size_t i = 0; i < size1; i++) {
        const auto idx = i * size2;
        const auto& p1 = patches1[i];

        for (size_t j = 0; j < size2; j++) {
            const auto& p2 = patches2[j];
            out[idx + j] = chebyshev_distance(p1.pos, p2.pos) < max_distance;
        }
    }

    return out;
}

void find_best_masks(
    const Stage1Cache::Masks& masks, size_t mask_count, size_t i,
    std::pair<uint64_t, uint64_t> current_masks, uint32_t count, uint32_t& best
) {
    auto remaining = mask_count - i;
    if (count + remaining <= best) {
        return;
    }

    if (i == mask_count) {
        best = std::max(best, count);
        return;
    }

    auto& other_masks = masks[i];
    // if (!(other_masks.first & current_masks.first) && !(other_masks.second & current_masks.second)) {
    //     auto new_masks = std::make_pair(other_masks.first | current_masks.first, other_masks.second | current_masks.second);
    //     find_best_masks(masks, mask_count, i + 1, new_masks, count + 1, best);
    // }
    if (!(other_masks.first & current_masks.first)) {
        auto new_masks = std::make_pair(other_masks.first | current_masks.first, other_masks.second | current_masks.second);
        find_best_masks(masks, mask_count, i + 1, new_masks, count + 1, best);
    }

    find_best_masks(masks, mask_count, i + 1, current_masks, count, best);
}

Finder<void>::EvalResult stage1_eval(
    const MapGenSettings&, const NoisePrecompute& precompute, NoiseCache& noise_cache, uint32_t seed, void*, Stage1Cache& cache
) {
    Patches patches = regular_patches(precompute, noise_cache, seed, { 0, 0 });
    transform_patches(patches[IRON], MIN_IRON_AREA, MAX_IRON_AREA);
    transform_patches(patches[COPPER], MIN_COPPER_AREA, MAX_COPPER_AREA);
    transform_patches(patches[COAL], MIN_COAL_AREA, MAX_COAL_AREA);
    transform_patches(patches[STONE], MIN_STONE_AREA, MAX_STONE_AREA);

    auto irons_coppers = make_pairs(patches[IRON], patches[COPPER], MAX_IRON_COPPER_DISTANCE);
    auto irons_coals = make_pairs(patches[IRON], patches[COAL], MAX_IRON_COAL_DISTANCE);
    auto irons_stones = make_pairs(patches[IRON], patches[STONE], MAX_IRON_STONE_DISTANCE);
    auto coppers_coals = make_pairs(patches[COPPER], patches[COAL], MAX_COPPER_COAL_DISTANCE);
    auto coppers_stones = make_pairs(patches[COPPER], patches[STONE], MAX_COPPER_STONE_DISTANCE);
    auto coals_stones = make_pairs(patches[COAL], patches[STONE], MAX_COAL_STONE_DISTANCE);

    size_t iron_size = patches[IRON].size();
    size_t copper_size = patches[COPPER].size();
    size_t coal_size = patches[COAL].size();
    size_t stone_size = patches[STONE].size();

    auto& masks = cache.masks;
    size_t mask_count = 0;

    for (size_t iron = 0; iron < iron_size; iron++) {
        for (size_t copper = 0; copper < copper_size; copper++) {
            if (!irons_coppers[iron*copper_size + copper]) continue;

            for (size_t coal = 0; coal < coal_size; coal++) {
                if (!irons_coals[iron*coal_size + coal] || !coppers_coals[copper*coal_size + coal]) continue;

                for (size_t stone = 0; stone < stone_size; stone++) {
                    if (
                        !irons_stones[iron*stone_size + stone] ||
                        !coppers_stones[copper*stone_size + stone] ||
                        !coals_stones[coal*stone_size + stone]
                    ) continue;

                    masks[mask_count] = std::make_pair(
                        (uint64_t)1 << iron | (uint64_t)1 << 32 << copper,
                        (uint64_t)1 << coal | (uint64_t)1 << 32 << stone
                    );
                    mask_count++;
                }
            }
        }
    }

    uint32_t best = 0;
    find_best_masks(masks, mask_count, 0, {0, 0}, 0, best);

    return { best < MIN_SCORE, (float)best };
}