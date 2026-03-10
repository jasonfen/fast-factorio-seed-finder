#pragma once

#define _USE_MATH_DEFINES
#include "noise.hpp"

// -/+ 20% approximation
float union_area(const PatchArray& patches);

bool is_point_in_patch(const PatchArray&, PositionI32 position);

// Will overestimate of patches are overlapping
template<typename T>
std::pair<float, Box<T>> count_ore_lines(const PatchArray& patches, Box<T> box, T max_width, Direction belt_direction) {
    T max_width_2 = max_width / 2;
    PatchArray valid_coppers;
    for (const auto& p : patches) {
        if (box.contains(p.pos)) valid_coppers.insert(p);
    }

    float best_count = 0.f;
    Box<T> best_bounding_box;
    for (size_t i = 0; i < valid_coppers.size(); i++) {
        float count = 0.f;
        Box<T> bounding_box = Box<T>(INT16_MAX, INT16_MAX, INT16_MIN, INT16_MIN);

        const auto& p1 = valid_coppers[i];
        for (size_t j = i; j < valid_coppers.size(); j++) {
            const auto& p2 = valid_coppers[j];
            int32_t forward1, forward2;
            if (belt_direction == EAST || belt_direction == WEST) {
                forward1 = p1.pos.x;
                forward2 = p2.pos.x;
            } else {
                forward1 = p1.pos.y;
                forward2 = p2.pos.y;
            }

            if (std::abs(forward1 - forward2) < max_width_2) {
                float scaled_radius = p2.radius + 4;
                if (scaled_radius <= 22.5) continue;

                count += 2.f / 7 * std::sqrt(scaled_radius*scaled_radius - 22.5f*22.5f);
                bounding_box = Box<T>::combine(bounding_box, Box<T>(Position<T>(p2.pos), (T)p2.radius));
            }
        }   

        if (best_count < count) {
            best_count = count;
            best_bounding_box = bounding_box;
        }
    }

    return { best_count, best_bounding_box };
}