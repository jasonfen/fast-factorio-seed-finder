# Unified seed finder: one binary, goal selected with --finder and map settings
# with --mode. To add a goal, drop a module under finders/ and list it in
# registry.cpp; no new executable or CMake target is needed.

add_executable(seed_finder
    main.cpp
    registry.hpp
    registry.cpp
    presets.hpp
    presets.cpp
    finders/peninsula_detect.hpp
    finders/peninsula.hpp
    finders/peninsula.cpp
    finders/peninsula_resources.hpp
    finders/peninsula_resources.cpp
)

# So that "registry.hpp" and "finders/foo.hpp" resolve from any TU here.
target_include_directories(seed_finder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(seed_finder PRIVATE core)

set_property(TARGET seed_finder PROPERTY CXX_STANDARD 23)
