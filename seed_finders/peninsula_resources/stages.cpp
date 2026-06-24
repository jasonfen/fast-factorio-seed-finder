#define _USE_MATH_DEFINES
#include "stages.hpp"

#include <array>

#include "noise.hpp"
#include "algorithm.hpp"

// ============================================================================
// Tunable criteria. These are the knobs you adjust per goal; the map settings
// (frequency/size/richness/water/elevation) live in the per-mode main_*.cpp.
// ============================================================================
namespace {

// --- Stage 1: spawn-resource gate (regular patches in the spawn region) ---

// Each of these ores must have a patch reaching within RESOURCE_RADIUS tiles of
// spawn (0,0) and be at least MIN_PATCH_RADIUS in size. Regular patches start
// ~250 tiles out, so RESOURCE_RADIUS should stay <= 512 (one region around spawn).
constexpr std::array<ResourceType, 4> REQUIRED_RESOURCES = { IRON, COPPER, COAL, STONE };
constexpr int32_t RESOURCE_RADIUS = 400;
constexpr float   MIN_PATCH_RADIUS = 12.f;   // ignore tiny slivers

// --- Stage 2: peninsula water detection (see detection_zones.png) ---

struct GridCell {
    int32_t x;
    int32_t y;
    constexpr bool operator==(const GridCell&) const = default;
};

// Marks the end of a ray (rays vary in length). No real cell uses negative
// coordinates, so this never collides with a sampled cell.
constexpr GridCell RAY_END{ -1, -1 };

// One eighth of a radial detection pattern on a 32x32 grid of cells. Each ray
// lists cells from the outer edge of the 1024x1024 search area inward toward
// spawn. The pattern is reflected into the other seven octants at runtime.
constexpr std::array<std::array<GridCell, 8>, 10> RAYS = {{
    {{ { 0,15}, { 0,14}, { 0,13}, { 0,12}, { 0,11}, { 0,10}, RAY_END, RAY_END }}, // red
    {{ { 2,15}, { 2,14}, { 1,13}, { 1,12}, { 1,11}, RAY_END, RAY_END, RAY_END }}, // green
    {{ { 3,15}, { 3,14}, { 2,13}, { 2,12}, { 2,11}, { 1,10}, RAY_END, RAY_END }}, // blue
    {{ { 4,15}, { 4,14}, { 3,13}, { 3,12}, { 3,11}, { 2,10}, RAY_END, RAY_END }}, // gray
    {{ { 6,15}, { 6,14}, { 5,13}, { 5,12}, { 4,11}, { 4,10}, { 3, 9}, RAY_END }}, // orange
    {{ { 7,15}, { 7,14}, { 6,13}, { 6,12}, { 5,11}, { 5,10}, { 4, 9}, RAY_END }}, // yellow
    {{ { 8,15}, { 8,14}, { 7,13}, { 7,12}, { 7,11}, { 6,10}, { 6, 9}, { 5, 8} }}, // lgreen
    {{ {10,15}, { 9,14}, { 9,13}, { 8,12}, { 8,11}, { 7,10}, { 7, 9}, { 6, 8} }}, // pink
    {{ {12,15}, {11,14}, {10,13}, {10,12}, { 9,11}, { 8,10}, RAY_END, RAY_END }}, // teal
    {{ {14,15}, {13,14}, {12,13}, {11,12}, {10,11}, { 9,10}, { 8, 9}, { 7, 8} }}  // dblue
}};

constexpr int32_t BOX_RADIUS = 16;             // Each cell is sampled as a 32x32 box (half-size 16).
constexpr int32_t SAMPLING_DISTANCE = 8;       // Within a box, check one tile every 8 tiles.
constexpr int32_t CELL_SIZE = BOX_RADIUS * 2;  // Cell spacing so boxes tile edge to edge.
constexpr int RAY_COUNT = (int)RAYS.size();
constexpr int OCTANTS = 8;

} // namespace

// Stage 1: keep only seeds whose regular patches put every required ore within
// reach of spawn at a usable size. Cheap (regular patches only), so it carries
// the bulk of the elimination. Score is the summed area of the qualifying
// patches, used only as a tie-break if more than seed_nb_to_next_stage survive.
Finder<>::EvalResult stage1_eval(
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
                ore_area += p.radius * p.radius;   // proportional to patch area
            }
        }
        if (!found) return { .eliminate = true, .score = 0.f };
    }

    return { .eliminate = false, .score = ore_area };
}

// Stage 2: score how enclosed by water the spawn is. For each of
// RAY_COUNT * OCTANTS (= 80) radial directions, walk outward-to-inward and award
// a point at the first cell containing water. Higher = more peninsula-like.
Finder<>::EvalResult stage2_eval(
    const MapGenSettings& settings, const NoisePrecompute& precompute, NoiseCache&, uint32_t seed, void*
) {
    Noise noise(seed, true, settings.elevation_type == ELEVATION_2_0);

    float score = 0.f;
    for (int octant = 0; octant < OCTANTS; octant++) {
        // Reflect the eighth-circle pattern into this octant: optionally swap
        // x/y and flip each axis sign (the three low bits of octant).
        const bool swap_xy  = (octant >> 0) & 1;
        const bool negate_x = (octant >> 1) & 1;
        const bool negate_y = (octant >> 2) & 1;

        for (const auto& ray : RAYS) {
            for (const GridCell cell : ray) {
                if (cell == RAY_END) break;

                const int32_t gx = swap_xy ? cell.x : cell.y;
                const int32_t gy = swap_xy ? cell.y : cell.x;
                const PositionI32 pos((negate_x ? -gx : gx) * CELL_SIZE,
                                      (negate_y ? -gy : gy) * CELL_SIZE);

                if (noise.any_water_in_box(settings, precompute, BoxI32(pos, BOX_RADIUS), SAMPLING_DISTANCE)) {
                    score++;
                    break; // The first water cell along this ray is enough.
                }
            }
        }
    }

    return { .eliminate = false, .score = score };
}
