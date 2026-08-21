cmake_minimum_required(VERSION 3.20)

project(OpenLRRModern
    VERSION 0.1.0
    LANGUAGES CXX
)

find_package(glfw3 REQUIRED)

add_executable(openlrr
    src/openlrr/linux/main.cpp
    src/openlrr/linux/Platform.cpp
)

target_include_directories(openlrr PRIVATE
    src/openlrr
)

target_compile_features(openlrr PRIVATE cxx_std_20)

target_link_libraries(openlrr PRIVATE glfw)

set_target_properties(openlrr PROPERTIES
    CXX_EXTENSIONS OFF
)
