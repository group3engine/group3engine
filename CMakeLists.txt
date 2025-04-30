cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(group3engine CXX C)


# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

# Check git submodules
# https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "${BoldCyan}Setting build type to '${default_build_type}' as none was specified.${ColourReset}")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "Platinum" "MinSizeRel" "RelWithDebInfo")
endif()

# Define flags for the Platinum build type
set(CMAKE_CXX_FLAGS_PLATINUM "${CMAKE_CXX_FLAGS_RELEASE} -static-libstdc++ -DPLATINUM" CACHE STRING "Flags for Platinum build type" FORCE)
set(CMAKE_C_FLAGS_PLATINUM "${CMAKE_C_FLAGS_RELEASE}  -DPLATINUM" CACHE STRING "Flags for Platinum build type" FORCE)

# Ensure Platinum is recognized as a valid build type
if(CMAKE_BUILD_TYPE STREQUAL "Platinum")
    message(STATUS "Configuring for Platinum build type")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_PLATINUM}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_PLATINUM}")
endif()

add_subdirectory(src)

set_target_properties(src PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

option(USE_WAYLAND "Use Wayland instead of X11" OFF)

if(APPLE)
  set(GLFW_BUILD_COCOA ON CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WIN32 OFF CACHE BOOL "" FORCE)
elseif(UNIX)
  set(GLFW_BUILD_COCOA OFF CACHE BOOL "" FORCE)
  if(USE_WAYLAND)
    set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
  else()
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
  endif()
  set(GLFW_BUILD_WIN32 OFF CACHE BOOL "" FORCE)
elseif(WIN32)
  set(GLFW_BUILD_COCOA OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WIN32 ON CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
  target_compile_definitions(src PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# Turn off unwanted SDL options
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/glfw)
add_subdirectory(extern/glm)
add_subdirectory(extern/spdlog)
add_subdirectory(extern/imgui)
add_subdirectory(extern/Vulkan-Headers)
add_subdirectory(extern/volk)
add_subdirectory(extern/SDL EXCLUDE_FROM_ALL)

# SoLoud
file(GLOB SOLOUD_CPP_SOURCE_FILES
    "extern/soloud/src/audiosource/wav/*.cpp"
    "extern/soloud/src/audiosource/wav/*.c"
    "extern/soloud/src/backend/miniaudio/*.cpp"
    "extern/soloud/src/core/*.cpp"
    "extern/soloud/src/filter/*.cpp"
)
target_sources(src PRIVATE ${SOLOUD_CPP_SOURCE_FILES})
# Include directories for IDEs - can just use target_sources with header files
target_include_directories(src PRIVATE
    extern/soloud/include
    extern/soloud/src/audiosource/wav
    extern/soloud/src/backend/miniaudio
)
target_compile_definitions(src PRIVATE WITH_MINIAUDIO)

option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(extern/tracy)

# Turn off rtti for Jolt
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
# Turn off all warnings in Jolt to stop -Werror build error in Release mode
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use static version of the MSVC runtime library with Jolt to match our use
  set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(extern/JoltPhysics/Build)
target_include_directories(src PUBLIC ${JoltPhysics_SOURCE_DIR}/..)

# Add header-only libraries
add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE extern/cgltf)

add_library(json INTERFACE)
target_include_directories(json INTERFACE extern/json)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE extern/vma)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE extern/stb_image)

target_link_libraries(volk PUBLIC Vulkan::Headers)

# The GLFW_INCLUDE_NONE macro will explicitly prevent the GLFW header from including the OpenGL header
# GLM_ENABLE_EXPERIMENTAL is necessary for #include <glm/gtx/transform.hpp>
target_compile_definitions(src PRIVATE GLFW_INCLUDE_NONE GLM_ENABLE_EXPERIMENTAL GLM_FORCE_XYZW_ONLY GLM_FORCE_QUAT_DATA_XYZW GLM_FORCE_QUAT_CTOR_XYZW GLM_FORCE_RADIANS GLM_FORCE_SIZE_T_LENGTH)

target_compile_definitions(src PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(src PRIVATE cgltf glfw glm::glm-header-only Jolt spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32> stb_image Tracy::TracyClient volk vma json SDL3::SDL3)

# Compiler warnings
if ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
  # Disable rtti with -fno-rtti to be compatible with Jolt
  target_compile_options(src PRIVATE -fno-rtti -Wall -Wextra -Wnon-virtual-dtor -pedantic -Wno-missing-field-initializers)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(src PRIVATE -fsignaling-nans)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Disable rtti with /GR- to be compatible with Jolt
  target_compile_options(src PRIVATE /GR- /permissive- /W4 /w14640)
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/shaders/*.vert" "${CMAKE_SOURCE_DIR}/src/shaders/*.frag" "${CMAKE_SOURCE_DIR}/src/shaders/*.comp")
file(GLOB_RECURSE GLSL_HEADER_FILES "${CMAKE_SOURCE_DIR}/src/shaders/*.glsl")

if(APPLE)
  set(GLSLC "${CMAKE_SOURCE_DIR}/extern/shaderc/macos-x86_64/glslc")
elseif(UNIX)
  set(GLSLC "${CMAKE_SOURCE_DIR}/extern/shaderc/linux-x86_64/glslc")
elseif(WIN32)
  set(GLSLC "${CMAKE_SOURCE_DIR}/extern/shaderc/win-x86_64/glslc.exe")
endif()

# Compile shaders
# https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396
set(SPIRV_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/assets/shaders/")
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(STEM ${GLSL} NAME_WLE)
  get_filename_component(EXTENSION ${GLSL} EXT)
  set(SPIRV "${SPIRV_OUTPUT_DIR}${STEM}${EXTENSION}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SPIRV_OUTPUT_DIR}"
    COMMAND ${GLSLC} -O --target-env=vulkan1.3 ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL} ${GLSL_HEADER_FILES})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()
add_custom_target(compile_shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(src compile_shaders)

# Copy asset files to the binary directory in Platinum build to replicate install
if(CMAKE_BUILD_TYPE STREQUAL "Platinum")
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Install
include(GNUInstallDirs)

install(TARGETS src)
install(DIRECTORY assets/ DESTINATION assets)
