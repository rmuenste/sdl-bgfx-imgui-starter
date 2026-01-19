# Building

This repo uses a superbuild to fetch and build SDL, bgfx, and Dear ImGui.
The examples below assume a Unix Makefiles generator and a Release build.

## Prerequisites (Ubuntu 24.04 / WSL)

Install the required development libraries:

```bash
sudo apt-get install build-essential cmake ninja-build libx11-dev libglu1-mesa-dev libgl1 libxext-dev libboost-all-dev
```

This installs:
- **build-essential** - C/C++ compiler toolchain (gcc, g++)
- **cmake** - Build system generator (version 3.24 or later required)
- **ninja-build** - Fast build system
- **libx11-dev** - X11 development libraries (windowing system)
- **libglu1-mesa-dev** - OpenGL Utility library
- **libgl1** - Mesa OpenGL runtime
- **libxext-dev** - X11 extension development libraries
- **libboost-all-dev** - Boost C++ libraries

## Superbuild configure

```bash
cmake -B build/release-make -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUPERBUILD=ON \
  -DBGFX_BUILD_EXAMPLES=ON
```

## Build the superbuild

```bash
cmake --build build/release-make
```

## Build bgfx examples

The bgfx examples are built inside the ExternalProject build tree.

```bash
cmake --build third-party/build/src/bgfx-build/Release --target examples
```

## Example output path

Example binaries land here:

```text
third-party/build/src/bgfx-build/Release/cmake/bgfx/example-00-helloworld
```

Run them from the bgfx runtime directory so assets resolve:

```bash
cd third-party/build/src/bgfx/bgfx/examples/runtime
../../../../build/src/bgfx-build/Release/cmake/bgfx/example-00-helloworld
```

## Reminder

This layout can be improved by adding a nicer output directory (for example,
copying or installing example binaries next to `examples/runtime`).
