file(GLOB_RECURSE CPP_SOURCE_FILES "src/common/*.c" "src/decompress/*.c")

add_library(zstd ${CPP_SOURCE_FILES})

target_compile_definitions(zstd PUBLIC ZSTD_DISABLE_ASM)

target_include_directories(zstd PUBLIC "include" "src" "src/common" "src/decompress")
