# Two executables sharing the same criteria (stages.cpp); they differ only in
# the per-mode map settings in their main_*.cpp.

add_executable(peninsula_resources_normal
    main_normal.cpp
    stages.hpp
    stages.cpp
)
target_link_libraries(peninsula_resources_normal PRIVATE core)
set_property(TARGET peninsula_resources_normal PROPERTY CXX_STANDARD 23)

add_executable(peninsula_resources_railworld
    main_railworld.cpp
    stages.hpp
    stages.cpp
)
target_link_libraries(peninsula_resources_railworld PRIVATE core)
set_property(TARGET peninsula_resources_railworld PROPERTY CXX_STANDARD 23)
