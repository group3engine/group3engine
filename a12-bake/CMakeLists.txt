file(GLOB_RECURSE CPP_SOURCE_FILES "*.cpp" "*.hpp" "*.h" "*.hxx")

add_executable(a12-bake ${CPP_SOURCE_FILES})

target_include_directories(a12-bake PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
