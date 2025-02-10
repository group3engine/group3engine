# Build and Run Instructions

`cmake -S . -B build`
`cmake --build build`
`./build/a12-bake/a12-bake`
`./build/a12/a12`

Configure build type:
`cmake -DCMAKE_BUILD_TYPE=Release -S . -B build`
`cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build`

Build in parallel:
`cmake --build build -j`

NOTE: You might have a CMake version that is too low for some of these CMake commands.
But you should install a CMake version of at least 3.20 (what the lab machines should have).

Also, I need to test if the flag -DCMAKE_INSTALL_PREFIX is needed on the lab machines.

Untested on Mac and Windows.
