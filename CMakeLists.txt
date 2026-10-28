cmake_minimum_required(VERSION 3.16)
project(atom-orbital-sim VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -Wall -Wextra -Wno-unused-parameter)
endif()

# Find external dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

# Set runtime output directory so executables live in bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# -----------------------------------------------------------------------------
# Dear ImGui library
# -----------------------------------------------------------------------------
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# -----------------------------------------------------------------------------
# Shared Core Simulation & Rendering Engine
# -----------------------------------------------------------------------------
set(ATOM_CORE_SOURCES
    src/renderer_particles.cpp
    src/renderer_raytracer.cpp
    src/renderer_excitation.cpp
    src/renderer_wave_2d.cpp
    src/renderer_bohr.cpp
    src/gui.cpp
)

add_library(atom_core STATIC ${ATOM_CORE_SOURCES})
target_include_directories(atom_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(atom_core PUBLIC
    OpenGL::GL
    glfw
    GLEW::glew
    glm::glm
    imgui
)

# -----------------------------------------------------------------------------
# 1. Master Application (Unified Interactive Simulator with Dear ImGui)
# -----------------------------------------------------------------------------
add_executable(atom_orbital_sim src/main.cpp)
target_link_libraries(atom_orbital_sim PRIVATE atom_core)

# -----------------------------------------------------------------------------
# Standalone Focused Binaries (Matching original project structure)
# -----------------------------------------------------------------------------
add_executable(atom_realtime src/standalone/atom_realtime_main.cpp)
target_link_libraries(atom_realtime PRIVATE atom_core)

add_executable(atom_raytracer src/standalone/atom_raytracer_main.cpp)
target_link_libraries(atom_raytracer PRIVATE atom_core)

add_executable(atom_excitation src/standalone/atom_excitation_main.cpp)
target_link_libraries(atom_excitation PRIVATE atom_core)

add_executable(wave_atom_2d src/standalone/wave_atom_2d_main.cpp)
target_link_libraries(wave_atom_2d PRIVATE atom_core)

add_executable(atom_bohr src/standalone/atom_bohr_main.cpp)
target_link_libraries(atom_bohr PRIVATE atom_core)

# -----------------------------------------------------------------------------
# Terminal Menu Launcher
# -----------------------------------------------------------------------------
add_executable(launcher src/standalone/launcher.cpp)

# -----------------------------------------------------------------------------
# Unit Tests (Pure analytical math verification)
# -----------------------------------------------------------------------------
add_executable(test_orbital_math tests/test_orbital_math.cpp)
target_include_directories(test_orbital_math PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Copy shaders into bin/shaders on build so running from bin/ or root works seamlessly
add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
    COMMENT "Copying shaders to bin/ directory"
)
add_dependencies(atom_orbital_sim copy_shaders)
