# README
## Update git submodules

`git submodule update --init --recursive`

*NOTE: CMake will give an error if this has not been run and then will run the command for you.*

Rerun CMake after this error.

## Build and Run Instructions

`cmake -S . -B build`  
`cmake --build build`  
`./build/src/src`  

### Configure build type:

`cmake -DCMAKE_BUILD_TYPE=Release -S . -B build`  
`cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build`  

### Build in parallel:
`cmake --build build -j`  

*REQUIRED: CMake 3.20.0 or higher.*