#define _USE_MATH_DEFINES
#include <cmath>
#include "noise.hpp"
#include <chrono>
#include <print>
#include <iostream>
#include <fstream>

#pragma warning( push, 0 )
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#pragma warning( pop )

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::println("You must specify a seed.");
        return 1;
    }

    uint32_t seed0 = std::stoul(std::string(argv[1]));
    std::println("seed: {}", seed0);

    constexpr int IMG_SIZE = 896;

    MapGenSettings settings;
    for (ResourceType t = IRON; t < NB_RESOURCE_TYPE; ++t) {
        settings.frequencies[t] = 6.f;
        settings.sizes[t] = 6.f;
        settings.richness[t] = 6.f;
    }
    settings.elevation_type = ELEVATION_ISLAND;
    settings.water_scale = 1.f/2;
    settings.water_coverage = 2;
    NoisePrecompute precompute(settings);
    Noise noise(seed0, true, true);
    NoiseCache cache;

    auto regular = regular_patches(precompute, cache, seed0, { 0, 0 });
    auto starter = starter_patches(settings, precompute, noise, cache, seed0, { 0, 0 });
    std::array<PatchArray, 9> biters;
    for (int i = 0; i < 9; i++) {
        biters[i] = enemy_bases(settings, precompute, seed0, { i % 3 - 1, i / 3 - 1 });
    }

    // Output buffer: RGB bytes
    std::vector<unsigned char> img(IMG_SIZE * IMG_SIZE * 3, 255);

    const int shift = IMG_SIZE / 2 ;

    for (int py = 0; py < IMG_SIZE; ++py) {
        for (int px = 0; px < IMG_SIZE; ++px) {
            // Map pixel -> world coordinates used by generate_candidates
            float wx = (float)(px - shift);
            float wy = (float)(py - shift);
            size_t idx = (py * IMG_SIZE + px) * 3;
            size_t biter_idx = ((size_t)wx + 256 + 512) / 512 + ((size_t)wy + 256 + 512) / 512 * 3;

            if (noise.is_tile_water(settings, precompute, { wx, wy })) {
                img[idx+0] = 51; img[idx+1] = 83; img[idx+2] = 95;
                continue;
            }

            float best_val = -std::numeric_limits<float>::infinity();
            ResourceType best_type = NB_RESOURCE_TYPE;

            auto check_patches = [&](const Patches& patches) {
                for (ResourceType type = IRON; type < NB_RESOURCE_TYPE; ++type) {
                    float best_for_type = -std::numeric_limits<float>::infinity();
                    for (auto it = patches[type].begin(); it != patches[type].end(); ++it) {
                        const Patch &p = *it;
                        float dist = PositionF32::distance({ wx, wy }, p.pos);
    
                        float slope = 0.f;
                        if (p.radius > 0.f) slope = 3.f * p.quantity / float(M_PI * p.radius * p.radius * p.radius);
    
                        float val = (p.radius - dist) * slope;
                        if (val > best_for_type) best_for_type = val;
                    }
    
                    if (best_for_type > best_val) {
                        best_val = best_for_type;
                        best_type = type;
                    }
                }
            };

            check_patches(starter);
            check_patches(regular);

            if (best_val <= 0.f || best_type == NB_RESOURCE_TYPE) {
                img[idx+0] = 55; img[idx+1] = 53; img[idx+2] = 11;
            } else {
                switch (best_type) {
                    case IRON:   img[idx+0]=105; img[idx+1]=133; img[idx+2]=147; break; // blue
                    case COPPER: img[idx+0]=204; img[idx+1]=98;  img[idx+2]=54;  break; // red
                    case COAL:   img[idx+0]=0;   img[idx+1]=0;   img[idx+2]=0;   break; // black
                    case STONE:  img[idx+0]=175; img[idx+1]=155; img[idx+2]=108; break; // gray
                    case OIL:    img[idx+0]=198; img[idx+1]=51;  img[idx+2]=196; break; // purple
                    default:     img[idx+0]=255; img[idx+1]=255; img[idx+2]=255; break;
                }
            }

            for (const auto& p : biters[biter_idx]) {
                float dist = PositionF32::distance({ wx, wy }, p.pos);

                float slope = 0.f;
                if (p.radius > 0.f) slope = 3.f * p.quantity / float(M_PI * p.radius * p.radius * p.radius);

                float val = (p.radius - dist) * slope;
                if (val > 0) {
                    img[idx+0] = 255; img[idx+1] = 25; img[idx+2] = 25;
                }
            }
        }
    }

    stbi_write_png("./img.png", IMG_SIZE, IMG_SIZE, 3, img.data(), 3*IMG_SIZE*sizeof(unsigned char));
    std::cout << "img.png\n";

    return 0;
}