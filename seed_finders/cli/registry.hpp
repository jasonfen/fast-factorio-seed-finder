#pragma once

#include <functional>
#include <string>
#include <vector>

#include "finder.hpp"   // StandardOptions, MapGenSettings

// One selectable goal in the unified binary. Each finder knows how to wire up
// its own stages given the chosen map settings, then run them.
struct FinderEntry {
    std::string name;
    std::string description;
    // Build the finder for the given map settings and run it to completion.
    std::function<int(const MapGenSettings& settings, const StandardOptions& options)> run;
};

// All finders compiled into this binary, selectable with --finder.
const std::vector<FinderEntry>& finder_registry();
