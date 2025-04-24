file(GLOB CPP_SOURCE_FILES "*.cpp" "*.hpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(src ${CPP_SOURCE_FILES} "rendering/IrradianceMap.cpp")

add_subdirectory(core)
add_subdirectory(physics)
add_subdirectory(rendering)
add_subdirectory(scene)
add_subdirectory(vkutil)
add_subdirectory(animation)
add_subdirectory(entities)
add_subdirectory(samples)
add_subdirectory(UI)

target_include_directories(src PRIVATE ${CMAKE_CURRENT_LIST_DIR})
