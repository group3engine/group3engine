file(GLOB_RECURSE CPP_SOURCE_FILES "*.cpp" "*.hpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(src ${CPP_SOURCE_FILES})

add_subdirectory(physics)

target_include_directories(src PRIVATE ${CMAKE_CURRENT_LIST_DIR})
