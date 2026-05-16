add_executable(example
    main.cpp
    stages.hpp
    stages.cpp
)

target_link_libraries(example PRIVATE core)

set_property(TARGET example PROPERTY CXX_STANDARD 23)