cmake_minimum_required(VERSION 3.20)

project(OpenLRRModern
    VERSION 0.1.0
    LANGUAGES CXX
)

add_executable(openlrr
    src/openlrr/linux/main.cpp
)

target_compile_features(openlrr PRIVATE cxx_std_20)

set_target_properties(openlrr PROPERTIES
    CXX_EXTENSIONS OFF
)
