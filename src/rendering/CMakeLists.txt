file(GLOB CPP_SOURCE_FILES "*.cpp" "*.hpp")

target_sources(src PRIVATE ${CPP_SOURCE_FILES})

target_include_directories(src PRIVATE ${CMAKE_CURRENT_LIST_DIR})
