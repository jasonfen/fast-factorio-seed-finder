#pragma once

#define _USE_MATH_DEFINES
#include "noise.hpp"

// -/+ 20% approximation
float union_area(const PatchArray& patches);

bool is_point_in_patch(const PatchArray&, PositionI32 position);

// Returns a number between 0 and 1;
float area_covered_by_patch(const PatchArray&, BoxI32, int32_t sampling_distance);

// Will overestimate of patches are overlapping
std::pair<float, BoxI32> count_ore_lines(const PatchArray& patches, BoxI32 box, int32_t max_width, Direction belt_direction);