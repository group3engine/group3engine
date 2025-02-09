file(GLOB_RECURSE CPP_SOURCE_FILES "*.cpp" "*.hpp" "*.h" "*.hxx")

add_library(gltf ${CPP_SOURCE_FILES})

target_include_directories(gltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
