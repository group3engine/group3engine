file(GLOB_RECURSE CPP_SOURCE_FILES "*.cpp" "*.hpp" "*.h" "*.hxx")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(a12 ${CPP_SOURCE_FILES})
