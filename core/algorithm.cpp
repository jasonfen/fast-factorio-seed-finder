#include "algorithm.hpp"

const float UNION_FACTOR = 4.f * std::sqrtf(2) / 3.f;
float union_area(const PatchArray& patches) {
    float sum = 0;

    for (const auto& p : patches) {
        sum += (float)M_PI * p.radius*p.radius;
    }

    for (size_t i = 0; i < patches.size(); i++) {
        const auto& p1 = patches[i];
        for (size_t j = i + 1; j < patches.size(); j++) {
            const auto& p2 = patches[j];
            float rr = p1.radius + p2.radius;

            float distance_2 = PositionI32::distance_2(p1.pos, p2.pos);
            if (distance_2 > rr*rr) continue;
            
            float distance = std::sqrt(distance_2);
            float w = rr - distance;
            float union_ = UNION_FACTOR * std::sqrt(w*w*w * p1.radius * p2.radius / rr);
            sum -= union_;
        }
    }

    return sum;
}

bool is_point_in_patch(const PatchArray& patches, PositionI32 position) {
    for (const auto& p : patches) {
        if (PositionI32::distance_2(p.pos, position) <= p.radius*p.radius) return true;
    }

    return false;
}