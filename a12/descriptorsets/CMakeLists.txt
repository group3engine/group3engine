file(GLOB CPP_SOURCE_FILES "*.cpp" "*.hpp")

target_sources(a12 PRIVATE ${CPP_SOURCE_FILES})

target_include_directories(a12 PUBLIC ${CMAKE_CURRENT_LIST_DIR})
