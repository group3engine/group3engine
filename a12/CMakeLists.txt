file(GLOB_RECURSE CPP_SOURCE_FILES "*.cpp" "*.hpp" "*.h" "*.hxx")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(a12 ${CPP_SOURCE_FILES})

target_include_directories(a12 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/descriptorsets
    ${CMAKE_CURRENT_SOURCE_DIR}/framebuffers
    ${CMAKE_CURRENT_SOURCE_DIR}/pipelines
    ${CMAKE_CURRENT_SOURCE_DIR}/renderpasses
)
