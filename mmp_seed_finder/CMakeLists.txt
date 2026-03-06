add_executable(mmp_seed_finder
    main.cpp
    stages.hpp
    stages.cpp
)

target_link_libraries(mmp_seed_finder PRIVATE core)

set_property(TARGET mmp_seed_finder PROPERTY CXX_STANDARD 23)