# Update git submodules

`git submodule update --init --recursive`

NOTE: CMake will give an error if this has not been run and then will run the command for you.

Rerun CMake after this error.

# Build and Run Instructions

`cmake -S . -B build`  
`cmake --build build`  
`./build/src/src`  

Configure build type:

`cmake -DCMAKE_BUILD_TYPE=Release -S . -B build`  
`cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build`  

Build in parallel:
`cmake --build build -j`  

NOTE: You might have a CMake version that is too low for some of these CMake commands.

But you should install a CMake version of at least 3.20 (what the lab machines should have).

Untested on Mac and Windows.
