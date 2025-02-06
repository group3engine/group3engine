cmake_minimum_required(VERSION 3.10)

# Project name
project(VulkanTime)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories for third-party projects
include_directories(
        ${CMAKE_SOURCE_DIR}/third_party/volk/include
        ${CMAKE_SOURCE_DIR}/third_party/vulkan/include
        ${CMAKE_SOURCE_DIR}/third_party/stb/include
        ${CMAKE_SOURCE_DIR}/third_party/glfw/include
        ${CMAKE_SOURCE_DIR}/third_party/VulkanMemoryAllocator/include
        ${CMAKE_SOURCE_DIR}/third_party/glm/include
        ${CMAKE_SOURCE_DIR}/third_party/rapidobj/include
        ${CMAKE_SOURCE_DIR}/third_party/tgen/include
        ${CMAKE_SOURCE_DIR}/third_party/zstd/include
)

# Define macros
add_definitions(
        -DGLM_FORCE_RADIANS=1
        -DGLM_FORCE_SIZE_T_LENGTH=1
        -DGLM_ENABLE_EXPERIMENTAL=1
        -DZSTD_DISABLE_ASM=1
)

# Main executable
file(GLOB_RECURSE SOURCES
        ${CMAKE_SOURCE_DIR}/a12/*.cpp
        ${CMAKE_SOURCE_DIR}/labutils/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES}
        a12/StandardMesh.cpp
        a12/StandardMesh.hpp
        a12/Pipeline.hpp
        a12/PipelineBaseClass.hpp
        a12/PerFrameResource.cpp
        a12/PerFrameResource.hpp
        a12/Light.cpp
        a12/Light.hpp
        a12/glsl.hpp
        a12/ShadowLight.cpp
        a12/ShadowLight.hpp
        a12/PipelineBaseClass.cpp
        a12/Renderer.cpp
        a12/Renderer.hpp
        a12/renderpasses/postprocessrenderpass.cpp
        a12/renderpasses/renderpasstotexture.cpp
        a12/renderpasses/renderpasses.hpp
        a12/descriptorsets/descriptorsets.hpp
        a12/framebuffers/framebuffers.hpp
        a12/framebuffers/rendertexture.cpp
        a12/descriptorsets/scene.cpp
        a12/descriptorsets/postprocess.cpp
        a12/descriptorsets/bloom.cpp
        a12/descriptorsets/lighting.cpp
        a12/pipelines/pipeline.h
        a12/pipelines/forward.cpp
        a12/pipelines/postprocess.cpp
        a12/framebuffers/framebuffers.cpp
        gltf/ResourceManager.hpp
        gltf/ResourceManager.cpp
        gltf/MaterialManager.cpp
        gltf/MaterialManager.hpp
        gltf/MeshManager.cpp
        gltf/MeshManager.hpp
        gltf/GLTFImportStructs.hpp
)

add_executable(a12-bake a12-bake/main.cpp
        a12-bake/index_mesh.cpp
        a12-bake/index_mesh.hpp
        a12-bake/input_model.hpp
        a12-bake/load_model_obj.cpp
        a12-bake/load_model_obj.hpp
        a12-bake/zstdistream.cpp
        a12-bake/zstdistream.hpp
        a12-bake/matrixtoquaternion.hpp
)

# add warning flags in debug mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wreorder -pedantic -Werror -Wno-unused-parameter)
endif()

# Link precompiled third-party libraries
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_directories(a12-bake PRIVATE ${CMAKE_SOURCE_DIR}/lib)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME}
            labutils-debug-x64-gcc
            x-glfw-debug-x64-gcc
            x-stb-debug-x64-gcc
            x-tgen-debug-x64-gcc
            x-vma-debug-x64-gcc
            x-volk-debug-x64-gcc
            x-zstd-debug-x64-gcc
    )
    target_link_libraries(a12-bake
            labutils-debug-x64-gcc
            x-glfw-debug-x64-gcc
            x-stb-debug-x64-gcc
            x-tgen-debug-x64-gcc
            x-vma-debug-x64-gcc
            x-volk-debug-x64-gcc
            x-zstd-debug-x64-gcc
    )
else()
    target_link_libraries(${PROJECT_NAME}
            labutils-release-x64-gcc
            x-glfw-release-x64-gcc
            x-stb-release-x64-gcc
            x-tgen-release-x64-gcc
            x-vma-release-x64-gcc
            x-volk-release-x64-gcc
            x-zstd-release-x64-gcc
    )
    target_link_libraries(a12-bake
            labutils-release-x64-gcc
            x-glfw-release-x64-gcc
            x-stb-release-x64-gcc
            x-tgen-release-x64-gcc
            x-vma-release-x64-gcc
            x-volk-release-x64-gcc
            x-zstd-release-x64-gcc
    )

endif()