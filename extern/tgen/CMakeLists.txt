file(GLOB_RECURSE CPP_SOURCE_FILES "*.cpp" "*.hpp" "*.h" "*.hxx")

add_library(tgen ${CPP_SOURCE_FILES})

target_include_directories(tgen PUBLIC .)
