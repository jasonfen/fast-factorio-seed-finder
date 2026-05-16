#define _USE_MATH_DEFINES
#include "stages.hpp"

#include "noise.hpp"
#include "algorithm.hpp"

// Parameters settings, precompute and noise_cache are required by some functions in noise.hpp.
// seed and settings are the current seed, water settings and elevation type we are looking evaluating
Finder<SeedCache>::EvalResult stage1_eval(
    const MapGenSettings&, const NoisePrecompute& precompute, NoiseCache& noise_cache, uint32_t seed, SeedCache* seed_cache
) {
    // Gives a high score to seeds with little ore. Only looks at regular patches.

    // The game generates regular patches by regions (or chunks) of 1024x1024 tiles. Region (0, 0) is centered on spawn.
    // For example region (-1, 1) would be all patches in the box of center (-1024, 1024) and of side length 1024.
    Patches r_patches = regular_patches(precompute, noise_cache, seed, {0, 0});

    float area = union_area(r_patches);

    // The seed cache is used to pass data to the next stages.
    // If the next stage introduces the twin seed, water settings or elevation types, then the cache will get copied to each configuration.
    //
    // Note that if the SeedCache template is void then seed_cache will be a nullptr.
    seed_cache->stage1_area = area;

    // .eliminate is the pass-or-fail method to eliminate seeds I talked about in stages.hpp.
    // If it is set to true, the seed will not make it through to the next stage.
    // If set to false, then the top n method will be used. If n is greater than the number of seeds that
    // were not eliminated then all of them will make it to the next stage.

    // The score is always "higher is better" but here we want "lower area is better" so we take the negative of the area.
    return { .eliminate = false, .score = -area };
}

Finder<SeedCache>::EvalResult stage2_eval(
    const MapGenSettings& settings, const NoisePrecompute& precompute, NoiseCache& noise_cache, uint32_t seed, SeedCache* seed_cache, Stage2Cache &
) {
    // We do the same thing as stage1 but with stater patches.

    Noise noise(seed, true, false);
    Patches s_patches = starter_patches(settings, precompute, noise, noise_cache, seed);

    float area = union_area(s_patches);
    
    /**
     * Now let's imagine this stage needs to do some algorithm that requires a very big array.
     * For such arrays, allocating them on the stack is impossible and allocating in the heap every time is way to slow.
     * So this is where the stage cache comes in, it is allocated on the heap once and passed to the eval function.
     */

    /**
     * Some heavy computation involving stage_cache.big_array
     */


    // Eliminate the seed if the starter patches area is greater than 1/10th of the regular patches area (because why not!)
    return { .eliminate = area > seed_cache->stage1_area / 10, .score = -area };
}

Finder<SeedCache>::EvalResult stage3_eval(
    const MapGenSettings& settings, const NoisePrecompute& precompute, NoiseCache& noise_cache, uint32_t seed, SeedCache*
) {
    // This stage tries to see if there is any water under patches and reduce the area accordingly

    // Here we regenerate patches, it could be possible to transmit this from previous stages using the noise_cache.
    // However it would probably not be faster because it would require much more memory.
    Noise noise(seed, true, settings.elevation_type == ELEVATION_2_0);
    Patches s_patches = starter_patches(settings, precompute, noise, noise_cache, seed);
    Patches r_patches = regular_patches(precompute, noise_cache, seed, {0, 0});

    auto transform_patches = [&](Patch &p, ResourceType, size_t) {
        // Here we just reduce the radius if the center is in water. This is very much inexact but it's an example so I don't really care
        if (noise.is_tile_water(settings, precompute, p.pos)) {
            p.radius /= 2;
        }
    };

    iterate_patches(s_patches, transform_patches);
    iterate_patches(r_patches, transform_patches);

    float area = union_area(s_patches) + union_area(r_patches);

    return { false, -area };
}