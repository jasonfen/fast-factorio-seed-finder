#include "registry.hpp"

#include "finders/peninsula.hpp"
#include "finders/peninsula_resources.hpp"

const std::vector<FinderEntry>& finder_registry() {
    // To add a finder: write a module under finders/ that exposes an entry()
    // and list it here. Geometry-baked finders (solo_any, mmp, ...) can be added
    // the same way; they keep their own seed-cache type inside their builder.
    static const std::vector<FinderEntry> registry = {
        peninsula::entry(),
        peninsula_resources::entry(),
    };
    return registry;
}
