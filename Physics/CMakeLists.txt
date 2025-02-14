cmake_minimum_required(VERSION 3.20)

file(GLOB CPP_SOURCE_FILES "*.cpp" "*.hpp")

project(Physics)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PHYSICS_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../extern/JoltPhysics)
include(${PHYSICS_REPO_ROOT}/Jolt/Jolt.cmake)

add_executable(Physics ${CPP_SOURCE_FILES})

target_link_libraries(Physics PRIVATE Jolt)

target_include_directories(Physics PRIVATE ${CMAKE_CURRENT_LIST_DIR})
