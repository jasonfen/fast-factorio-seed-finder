#pragma once

#include <array>

#include "noise.hpp"

// Shared "how enclosed by water is this spawn?" scorer, used by both the
// peninsula and peninsula_resources finders. See detection_zones.png for the
// ray pattern.
namespace peninsula_detect {

struct GridCell {
    int32_t x;
    int32_t y;
    constexpr bool operator==(const GridCell&) const = default;
};

// Marks the end of a ray (rays vary in length). No real cell uses negative
// coordinates, so this never collides with a sampled cell.
inline constexpr GridCell RAY_END{ -1, -1 };

// One eighth of a radial detection pattern on a 32x32 grid of cells. Each ray
// lists cells from the outer edge of the 1024x1024 search area inward toward
// spawn; the pattern is reflected into the other seven octants at runtime.
inline constexpr std::array<std::array<GridCell, 8>, 10> RAYS = {{
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

inline constexpr int32_t BOX_RADIUS = 16;             // Each cell is sampled as a 32x32 box.
inline constexpr int32_t SAMPLING_DISTANCE = 8;       // Check one tile every 8 tiles in a box.
inline constexpr int32_t CELL_SIZE = BOX_RADIUS * 2;  // Cell spacing so boxes tile edge to edge.
inline constexpr int RAY_COUNT = (int)RAYS.size();
inline constexpr int OCTANTS = 8;

// Score in [0, RAY_COUNT * OCTANTS] (= 80). For each radial direction, walk
// outward-to-inward and award a point at the first cell containing water. Higher
// means more directions are blocked by water, i.e. a more peninsula-like spawn.
inline float score(Noise& noise, const MapGenSettings& settings, const NoisePrecompute& precompute) {
    float total = 0.f;
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
                    total++;
                    break; // The first water cell along this ray is enough.
                }
            }
        }
    }
    return total;
}

} // namespace peninsula_detect
