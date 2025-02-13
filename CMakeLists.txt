cmake_minimum_required(VERSION 3.19...3.31)

project(group3engine)

add_subdirectory(src)

set_target_properties(src PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

option(USE_WAYLAND "Use Wayland instead of X11" OFF)

if(APPLE)
  target_compile_definitions(src PRIVATE VK_USE_PLATFORM_METAL_EXT)
  target_compile_definitions(src PRIVATE GLFW_EXPOSE_NATIVE_COCOA)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_METAL_EXT)
elseif(UNIX)
  if(USE_WAYLAND)
    target_compile_definitions(src PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
    target_compile_definitions(src PRIVATE GLFW_EXPOSE_NATIVE_WAYLAND)
    set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
  else()
    target_compile_definitions(src PRIVATE VK_USE_PLATFORM_XLIB_KHR)
    target_compile_definitions(src PRIVATE GLFW_EXPOSE_NATIVE_X11)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
  endif()
elseif(WIN32)
  target_compile_definitions(src PRIVATE VK_USE_PLATFORM_WIN32_KHR GLFW_EXPOSE_NATIVE_WIN32)
  target_compile_definitions(src PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  set(VOLK_STATIC_DEFINES "VK_USE_PLATFORM_WIN32_KHR")
endif()

add_subdirectory(extern/glfw)
add_subdirectory(extern/glm)
add_subdirectory(extern/spdlog)
add_subdirectory(extern/volk)

# Add header-only libraries
add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE extern/cgltf)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE extern/vma)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE extern/stb_image)

# The GLFW_INCLUDE_NONE macro will explicitly prevent the GLFW header from including the OpenGL header
# GLM_ENABLE_EXPERIMENTAL is necessary for #include <glm/gtx/transform.hpp>
target_compile_definitions(src PRIVATE GLFW_INCLUDE_NONE GLM_ENABLE_EXPERIMENTAL)

target_link_libraries(src PRIVATE cgltf glfw glm::glm-header-only spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32> stb_image volk vma)

file(GLOB_RECURSE GLSL_SOURCE_FILES "src/shaders/*.vert" "src/shaders/*.frag")
file(GLOB_RECURSE GLSL_HEADER_FILES "src/shaders/*.glsl")

if(APPLE)
  set(GLSLC "${CMAKE_SOURCE_DIR}/extern/shaderc/macos-x86_64/glslc")
elseif(UNIX)
  set(GLSLC "${CMAKE_SOURCE_DIR}/extern/shaderc/linux-x86_64/glslc")
elseif(WIN32)
  set(GLSLC "${CMAKE_SOURCE_DIR}/extern/shaderc/win-x86_64/glslc.exe")
endif()

# Compile shaders
# https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396
set(SPIRV_OUTPUT_DIR "../assets/shaders/")
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

# Check git submodules
# https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
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
