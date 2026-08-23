cmake_minimum_required(VERSION 3.20)

project(OpenLRRModern
    VERSION 0.1.0
    LANGUAGES CXX
)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(openlrr
    src/openlrr/linux/main.cpp
    src/openlrr/linux/Platform.cpp
    src/openlrr/input/InputBinding.cpp
    src/openlrr/input/InputActions.cpp
	src/openlrr/input/RenderCursor.cpp
    src/openlrr/renderer/VulkanRenderer.cpp
    src/openlrr/renderer/PresentationViewport.cpp
	src/openlrr/renderer/TextRenderer.cpp
    src/openlrr/dev/RendererDiagnostics.cpp
	src/openlrr/dev/GuiDiagnostics.cpp
    src/openlrr/gui/GuiLayer.cpp
	
)

target_include_directories(openlrr PRIVATE
    src/openlrr
)

target_compile_features(openlrr PRIVATE cxx_std_20)

target_link_libraries(openlrr PRIVATE
    glfw
    Vulkan::Vulkan
)

set_target_properties(openlrr PROPERTIES
    CXX_EXTENSIONS OFF
)
