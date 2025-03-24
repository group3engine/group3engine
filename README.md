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

## Tracy build instructions

### Build Tracy Server

`cd extern/tracy`  
`cmake -DLEGACY=1 -B profiler/build -S profiler -DCMAKE_BUILD_TYPE=Release`  
`cmake --build profiler/build --config Release --parallel`

#### Notes

You may get some build errors if you are missing Linux packages. Try to install
any missing ones. (This should only happen on non lab machine Linux devices.)

### Run Tracy Server

`cd extern/tracy`  
`./profiler/build/tracy-profiler`

### Tracy Client Notes

See the Tracy docs at https://github.com/wolfpld/tracy.

#### Windows

> If you are using MSVC, you will need to disable the Edit And Continue feature,
> as it makes the compiler non-conformant to some aspects of the C++ standard.
> In order to do so, open the project properties and go to C/C++ General Debug
> Information Format and make sure Program Database for Edit And Continue (/ZI)
> is not selected.

> In MSVC, you would typically run your program using the Start Debugging menu
> option, which is conveniently available as a F5 shortcut. You should instead
> use the Start Without Debugging option, available as Ctrl + F5 shortcut.
