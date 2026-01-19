# Rebuilding Third-Party Dependencies

This document explains how to rebuild third-party dependencies when you make changes to them.

## Overview

The project uses a CMake superbuild to fetch and build third-party dependencies (SDL2, bgfx, imgui, PE). These are built as ExternalProjects and installed to:

```
third-party/build/
├── include/     # Installed headers (used by your code)
├── lib/         # Installed libraries (linked by your code)
└── src/         # Source code (where you make edits)
```

**Important**: When you edit source files in `third-party/build/src/`, you need to rebuild AND reinstall them so your examples pick up the changes.

## Rebuilding PE (Physics Engine)

PE is mostly a header-only library with some compiled code in `libpe.a`. When you modify PE source files, follow these steps:

### 1. Edit Source Files

Edit files in the source directory:
```
third-party/build/src/pe/pe/
```

For example:
- `pe/core/collisionsystem/HardContactAndFluid.h`
- `pe/core/collisionsystem/HardContactSemiImplicitTimesteppingSolvers.h`

### 2. Rebuild PE Library

```bash
cd third-party/build/src/pe-build/Release
ninja
```

This rebuilds `lib/libpe.a` with your changes.

### 3. Install Updated Files

Copy the rebuilt library and updated headers to the install directory:

```bash
cd /home/rapha/code/bgfx-rendering/sdl-bgfx-imgui-starter/third-party/build

# Copy library
cp src/pe-build/Release/lib/libpe.a lib/libpe.a

# Copy headers
cmake -E copy_directory src/pe/pe include/pe
```

### 4. Rebuild Your Examples

Force a rebuild of your examples to recompile against new headers and relink against the new library:

```bash
cd ../../build/release

# Force relink by removing the binary
rm -f examples/00-physics-basic/00-physics-basic

# Rebuild
ninja
```

### Automated Script

A convenience script is provided to automate the entire rebuild process:

```bash
./rebuild-pe.sh
```

This script will:
1. Rebuild the PE library
2. Copy the library to the install directory
3. Copy headers to the install directory
4. Rebuild your examples

### Manual Steps

If you prefer to run the steps manually (from project root):

```bash
# 1. Rebuild PE library
cd third-party/build/src/pe-build/Release
ninja

# 2. Install library and headers
cd ../..  # Now in third-party/build/src
cp pe-build/Release/lib/libpe.a ../lib/libpe.a
cmake -E copy_directory pe/pe ../include/pe

# 3. Rebuild examples
cd ../../../build/release  # Back to build/release
rm -f examples/00-physics-basic/00-physics-basic
ninja
```

## Why This is Necessary

The superbuild structure means:
1. **Your code compiles against** `third-party/build/include/` (installed headers)
2. **Your code links against** `third-party/build/lib/` (installed libraries)
3. **You edit code in** `third-party/build/src/` (source directory)

The install step (`cmake --build . --target install` in the ExternalProject) normally copies files from `src/` to `include/` and `lib/`. When you edit source files directly, you need to manually perform this install step.

## Rebuilding Other Dependencies

### SDL2

```bash
cd third-party/build/src/SDL2-build/Release
ninja
ninja install
```

### bgfx

```bash
cd third-party/build/src/bgfx-build/Release
ninja
ninja install
```

### imgui

```bash
cd third-party/build/src/imgui.cmake-build/Release
ninja
ninja install
```

## Verifying Changes Were Applied

Check if installed headers contain your changes:

```bash
grep -n "your_change" third-party/build/include/pe/core/collisionsystem/HardContactAndFluid.h
```

Check library timestamp:

```bash
ls -lh third-party/build/lib/libpe.a
```

The modification time should be recent if the rebuild worked.
