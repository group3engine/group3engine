file(GLOB CPP_SOURCE_FILES "*.cpp" "*.hpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(a12 ${CPP_SOURCE_FILES})

add_subdirectory(descriptorsets)
add_subdirectory(framebuffers)
add_subdirectory(pipelines)
add_subdirectory(renderpasses)
add_subdirectory(labutils)
add_subdirectory(gltf)

target_include_directories(a12 PRIVATE ${CMAKE_CURRENT_LIST_DIR})
