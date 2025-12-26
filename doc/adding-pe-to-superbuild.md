# Adding PE (Physics Engine) to the Superbuild

This document explains how to integrate the PE (Physics Engine) library into the bgfx-sdl-imgui superbuild configuration.

## Table of Contents
- [Understanding the Superbuild](#understanding-the-superbuild)
- [Current Superbuild Structure](#current-superbuild-structure)
- [Adding PE as an External Project](#adding-pe-as-an-external-project)
- [Integration Steps](#integration-steps)
- [PE Build Configuration](#pe-build-configuration)
- [Linking PE in Applications](#linking-pe-in-applications)
- [Complete Example](#complete-example)

## Understanding the Superbuild

### What is a Superbuild?

A superbuild is a CMake pattern that manages multiple external projects with complex dependencies. Instead of manually building and installing each dependency, the superbuild:

1. Downloads each dependency from Git (or other sources)
2. Configures each with appropriate CMake options
3. Builds each in the correct order (respecting dependencies)
4. Installs to a common prefix directory
5. Builds the main project with all dependencies available

### Benefits

- **Reproducible builds**: Same versions, same configuration every time
- **Simplified workflow**: One command builds everything
- **Dependency management**: CMake handles build order
- **Isolated build**: Third-party code separate from main project
- **Cross-platform**: Works on Windows, Linux, macOS, Emscripten

## Current Superbuild Structure

### File Organization

```
bgfx-sdl-imgui/
├── CMakeLists.txt              # Main project or superbuild entry point
├── cmake/
│   └── superbuild.cmake        # Superbuild target configuration
├── third-party/
│   ├── CMakeLists.txt          # Third-party dependencies (SDL2, bgfx, imgui)
│   └── build/                  # Generated: built dependencies
│       ├── include/            # Headers
│       ├── lib/                # Libraries
│       └── src/                # Source downloads
└── main.cpp                    # Application source
```

### Current Dependencies

**Location:** `third-party/CMakeLists.txt`

The superbuild currently manages three external projects:

1. **SDL2** (Lines 36-47)
   - Window management, input handling
   - Version: release-2.28.4
   - Skip on Emscripten (uses native browser APIs)

2. **bgfx** (Lines 49-63)
   - Cross-platform rendering library
   - Uses bgfx.cmake wrapper
   - Optional: Build examples with `BGFX_BUILD_EXAMPLES`

3. **imgui.cmake** (Lines 65-77)
   - Dear ImGui UI library
   - Uses imgui.cmake wrapper

### Build Flow

```
┌─────────────────────────────────────┐
│  cmake -DSUPERBUILD=ON ..           │
│  (Main CMakeLists.txt)              │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  third-party/CMakeLists.txt         │
│  Defines external projects:         │
│  - SDL2                             │
│  - bgfx                             │
│  - imgui.cmake                      │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  cmake/superbuild.cmake             │
│  Creates main project build         │
│  with dependencies installed        │
└─────────────────────────────────────┘
```

## Adding PE as an External Project

### PE Repository Information

Based on `pe.md`, the PE library:
- Is CMake-based (modern build system)
- Supports static/shared library builds
- Has optional dependencies (CGAL, MPI, OpenCL, Irrlicht, Eigen)
- Requires Boost (>=1.46.1)

### Required Information

To add PE to the superbuild, we need:

1. **Git repository URL**: Where is PE hosted?
   ```
   Example: https://github.com/username/pe-physics.git
   ```

2. **Git tag/commit**: Which version to use?
   ```
   Example: main, v1.0.0, or specific commit hash
   ```

3. **CMake options**: How to configure PE?
   ```cmake
   -DLIBRARY_TYPE=STATIC
   -DEXAMPLES=OFF          # Don't build PE examples in superbuild
   -DIRRLICHT=OFF          # We're using bgfx instead
   -DEIGEN=ON              # Enable Eigen support
   -DCGAL=ON               # Enable CGAL for mesh support
   -DUSE_JSON=ON           # Enable JSON support
   -DMPI=OFF               # Disable MPI for simplicity (enable if needed)
   -DOPENCL=OFF            # Disable OpenCL for simplicity
   ```

4. **Install behavior**: Does PE support `make install`?
   - If yes: Use standard install target
   - If no: May need custom install commands

## Integration Steps

### Step 1: Add PE to third-party/CMakeLists.txt

Add the PE external project after the existing dependencies:

```cmake
# At the end of third-party/CMakeLists.txt, after imgui.cmake:

# PE (Physics Engine) - Rigid body dynamics simulation
ExternalProject_Add(
  pe
  GIT_REPOSITORY https://github.com/username/pe-physics.git  # ← UPDATE THIS
  GIT_TAG main  # ← Or specific version tag
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/pe-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             -DLIBRARY_TYPE=STATIC
             -DEXAMPLES=OFF
             -DIRRLICHT=OFF
             -DEIGEN=ON
             -DCGAL=${PE_BUILD_CGAL}  # Make CGAL optional
             -DUSE_JSON=ON
             -DMPI=OFF
             -DOPENCL=OFF
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install ${build_config_arg}
)
```

### Step 2: Add PE CMake Option

In the main `CMakeLists.txt`, add an option to control CGAL support:

```cmake
# After the BGFX_BUILD_EXAMPLES option (around line 16):

option(PE_BUILD_CGAL "Build PE with CGAL support for triangle meshes" ON)
```

### Step 3: Update Dependency List

In `cmake/superbuild.cmake`, add PE to the dependency list:

```cmake
# Line 1: Add pe to dependencies
list(APPEND THIRD_PARTY_DEPENDENCIES bgfx imgui.cmake pe)
if (NOT EMSCRIPTEN)
  list(APPEND THIRD_PARTY_DEPENDENCIES SDL2)
endif ()
```

### Step 4: Handle Boost Dependency

PE requires Boost. Two approaches:

#### Option A: Assume System Boost (Simpler)

Require users to install Boost before building:

```cmake
# In third-party/CMakeLists.txt, before the PE ExternalProject_Add:

# PE requires Boost - must be installed on system
find_package(Boost 1.46.1 REQUIRED COMPONENTS thread system filesystem program_options)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "PE requires Boost >= 1.46.1. Please install Boost first.")
endif()
```

#### Option B: Build Boost in Superbuild (More Complex)

Add Boost as another external project (requires significant CMake code).

**Recommendation**: Start with Option A (system Boost).

### Step 5: Handle CGAL Dependency (If Enabled)

If `PE_BUILD_CGAL=ON`, CGAL must be available:

```cmake
# In third-party/CMakeLists.txt, before PE:

if (PE_BUILD_CGAL)
  find_package(CGAL QUIET)
  if (NOT CGAL_FOUND)
    message(WARNING "CGAL not found. PE will be built without CGAL support.")
    set(PE_BUILD_CGAL OFF)
  endif()
endif()
```

## PE Build Configuration

### Minimal Configuration (No Optional Dependencies)

This configuration works out-of-the-box with just Boost:

```cmake
ExternalProject_Add(
  pe
  GIT_REPOSITORY https://github.com/username/pe-physics.git
  GIT_TAG main
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/pe-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DLIBRARY_TYPE=STATIC
             -DEXAMPLES=OFF
             -DIRRLICHT=OFF       # Using bgfx instead
             -DEIGEN=ON           # Core dependency
             -DCGAL=OFF           # Optional: triangle mesh support
             -DUSE_JSON=ON        # Core dependency
             -DMPI=OFF            # Optional: parallel simulations
             -DOPENCL=OFF         # Optional: GPU acceleration
             -DBLAS=OFF           # Optional: linear algebra acceleration
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install ${build_config_arg}
)
```

### Full-Featured Configuration

Enable all optional features:

```cmake
ExternalProject_Add(
  pe
  # ... same as above ...
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DLIBRARY_TYPE=STATIC
             -DEXAMPLES=OFF
             -DIRRLICHT=OFF
             -DEIGEN=ON
             -DCGAL=ON            # ← Triangle mesh support
             -DUSE_JSON=ON
             -DMPI=ON             # ← Parallel simulations
             -DOPENCL=ON          # ← GPU acceleration
             -DBLAS=ON            # ← Linear algebra acceleration
  # ...
)
```

**Note**: Full configuration requires additional system dependencies:
- CGAL (with GMP, MPFR)
- MPI implementation (OpenMPI, MPICH)
- OpenCL SDK
- BLAS implementation (OpenBLAS, Intel MKL)

## Linking PE in Applications

### Update Main CMakeLists.txt

After the superbuild completes, PE will be available via `find_package`:

```cmake
# In main CMakeLists.txt, after line 33:

find_package(SDL2 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(bgfx REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(imgui.cmake REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(pe REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)  # ← Add this
```

### Link PE Library

```cmake
# Update target_link_libraries (around line 40):

target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  SDL2::SDL2-static
  SDL2::SDL2main
  bgfx::bgfx
  bgfx::bx
  imgui.cmake::imgui.cmake
  pe::pe  # ← Add this
)
```

### If PE Doesn't Export CMake Config

Some projects don't provide `peConfig.cmake`. If PE doesn't export a config file, manually specify include/library paths:

```cmake
# Instead of find_package(pe ...):

set(PE_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)
set(PE_LIBRARY ${CMAKE_PREFIX_PATH}/lib/libpe.a)  # Or .lib on Windows

target_include_directories(${PROJECT_NAME} PRIVATE ${PE_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PE_LIBRARY})
```

## Complete Example

### Full third-party/CMakeLists.txt with PE

```cmake
cmake_minimum_required(VERSION 3.24)

if (NOT SUPERBUILD)
  project(third-party)
endif ()

include(ExternalProject)

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT isMultiConfig)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
  endif ()
  set(build_type_dir ${CMAKE_BUILD_TYPE})
  set(build_type_arg -DCMAKE_BUILD_TYPE=$<CONFIG>)
else ()
  set(build_config_arg --config=$<CONFIG>)
endif ()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
  set(THIRD_PARTY_CMAKE_COMMAND emcmake cmake)
else ()
  set(THIRD_PARTY_CMAKE_COMMAND ${CMAKE_COMMAND})
endif ()

if (SUPERBUILD)
  set(PREFIX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/${THIRD_PARTY_BUILD_DIR_NAME})
else ()
  set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif ()

# ═══════════════════════════════════════════════════════════
# SDL2 - Window management (skip on Emscripten)
# ═══════════════════════════════════════════════════════════
if (NOT EMSCRIPTEN)
  ExternalProject_Add(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.4
    PREFIX ${PREFIX_DIR}
    BINARY_DIR ${PREFIX_DIR}/src/SDL2-build/${build_type_dir}
    CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
    CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
    INSTALL_COMMAND cmake --build <BINARY_DIR> --target install ${build_config_arg}
  )
endif ()

# ═══════════════════════════════════════════════════════════
# bgfx - Cross-platform rendering library
# ═══════════════════════════════════════════════════════════
ExternalProject_Add(
  bgfx
  GIT_REPOSITORY https://github.com/pr0g/bgfx.cmake.git
  GIT_TAG bde3f94ce75ffd184e9a3bdfe947a3bec69233eb
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/bgfx-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             "$<$<PLATFORM_ID:Emscripten>:-DBGFX_CONFIG_MULTITHREADED=OFF>"
             -DBGFX_BUILD_EXAMPLES=${BGFX_BUILD_EXAMPLES}
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install ${build_config_arg}
)

# ═══════════════════════════════════════════════════════════
# imgui.cmake - Dear ImGui UI library
# ═══════════════════════════════════════════════════════════
ExternalProject_Add(
  imgui.cmake
  GIT_REPOSITORY https://github.com/pr0g/imgui.cmake.git
  GIT_TAG 20e7d1a627690526c98b0b48c346e384ab87c5a6
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/imgui.cmake-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=ON
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install ${build_config_arg}
)

# ═══════════════════════════════════════════════════════════
# PE - Physics Engine (Rigid Body Dynamics)
# ═══════════════════════════════════════════════════════════

# PE requires Boost - must be installed on system
find_package(Boost 1.46.1 REQUIRED COMPONENTS thread system filesystem program_options)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "PE requires Boost >= 1.46.1. Please install Boost first.")
endif()

# Check for CGAL if enabled
if (PE_BUILD_CGAL)
  find_package(CGAL QUIET)
  if (NOT CGAL_FOUND)
    message(WARNING "CGAL not found. PE will be built without CGAL support.")
    set(PE_BUILD_CGAL OFF)
  endif()
endif()

ExternalProject_Add(
  pe
  GIT_REPOSITORY https://github.com/username/pe-physics.git  # ← UPDATE THIS
  GIT_TAG main  # ← Or specific version/tag
  PREFIX ${PREFIX_DIR}
  BINARY_DIR ${PREFIX_DIR}/src/pe-build/${build_type_dir}
  CMAKE_COMMAND ${THIRD_PARTY_CMAKE_COMMAND}
  CMAKE_ARGS ${build_type_arg}
             -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             # PE Configuration
             -DLIBRARY_TYPE=STATIC
             -DEXAMPLES=OFF            # Don't build PE examples
             -DIRRLICHT=OFF            # Using bgfx instead
             -DEIGEN=ON                # Linear algebra support
             -DCGAL=${PE_BUILD_CGAL}   # Triangle mesh support (optional)
             -DUSE_JSON=ON             # JSON support
             -DMPI=OFF                 # Parallel simulations (optional)
             -DOPENCL=OFF              # GPU acceleration (optional)
             -DBLAS=OFF                # BLAS acceleration (optional)
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND cmake --build <BINARY_DIR> --target install ${build_config_arg}
)
```

### Updated cmake/superbuild.cmake

```cmake
# Add pe to dependency list
list(APPEND THIRD_PARTY_DEPENDENCIES bgfx imgui.cmake pe)
if (NOT EMSCRIPTEN)
  list(APPEND THIRD_PARTY_DEPENDENCIES SDL2)
endif ()

ExternalProject_Add(
  ${CMAKE_PROJECT_NAME}_superbuild
  DEPENDS ${THIRD_PARTY_DEPENDENCIES}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
  CMAKE_ARGS
    -DCMAKE_PREFIX_PATH=${CMAKE_CURRENT_SOURCE_DIR}/third-party/${THIRD_PARTY_BUILD_DIR_NAME}
    -DSUPERBUILD=OFF
    ${build_type_arg}
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  INSTALL_COMMAND ""
)
```

### Updated Main CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.24)

option(SUPERBUILD "Perform a superbuild (or not)" OFF)

if (NOT DEFINED BGFX_BUILD_EXAMPLES)
  if (CMAKE_CONFIGURATION_TYPES)
    set(_bgfx_build_examples_default OFF)
  elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_bgfx_build_examples_default ON)
  else ()
    set(_bgfx_build_examples_default OFF)
  endif ()
  set(BGFX_BUILD_EXAMPLES ${_bgfx_build_examples_default}
      CACHE BOOL "Build bgfx examples in the superbuild")
  unset(_bgfx_build_examples_default)
endif ()

# ← ADD THIS: Option for PE CGAL support
option(PE_BUILD_CGAL "Build PE with CGAL support for triangle meshes" ON)

project(sdl-bgfx-imgui-starter LANGUAGES CXX)

if (SUPERBUILD)
  if (EMSCRIPTEN)
    set(THIRD_PARTY_BUILD_DIR_NAME embuild)
  else ()
    set(THIRD_PARTY_BUILD_DIR_NAME build)
  endif ()
  include(third-party/CMakeLists.txt)
  include(cmake/superbuild.cmake)
  return()
endif ()

# Find packages (PE added)
find_package(SDL2 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(bgfx REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(imgui.cmake REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(pe REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)  # ← ADD THIS

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
  main.cpp
  sdl-imgui/imgui_impl_sdl2.cpp
  bgfx-imgui/imgui_impl_bgfx.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)

# Link libraries (PE added)
target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  SDL2::SDL2-static
  SDL2::SDL2main
  bgfx::bgfx
  bgfx::bx
  imgui.cmake::imgui.cmake
  pe::pe  # ← ADD THIS
)

# ... rest of file unchanged ...
```

## Building with PE

### Prerequisites

Install Boost on your system:

**Ubuntu/Debian:**
```bash
sudo apt-get install libboost-all-dev
```

**macOS (Homebrew):**
```bash
brew install boost
```

**Windows (vcpkg):**
```cmd
vcpkg install boost-thread boost-system boost-filesystem boost-program-options
```

### Build Commands

```bash
# From repository root
mkdir build
cd build

# Configure with superbuild
cmake -DSUPERBUILD=ON -DCMAKE_BUILD_TYPE=Release ..

# Build everything (SDL2, bgfx, imgui, PE, and main app)
cmake --build .

# Run the application
./sdl-bgfx-imgui-starter  # Linux/Mac
# or
sdl-bgfx-imgui-starter.exe  # Windows
```

### Build with CGAL Support

```bash
# Install CGAL first
sudo apt-get install libcgal-dev  # Ubuntu
# or
brew install cgal  # macOS

# Configure with CGAL enabled
cmake -DSUPERBUILD=ON -DCMAKE_BUILD_TYPE=Release -DPE_BUILD_CGAL=ON ..
cmake --build .
```

### Troubleshooting

**Problem**: CMake can't find Boost
```
Solution: Set BOOST_ROOT environment variable:
export BOOST_ROOT=/path/to/boost
cmake -DSUPERBUILD=ON ..
```

**Problem**: PE doesn't have a CMake config file
```
Solution: Use manual include/library paths (see "If PE Doesn't Export CMake Config" section)
```

**Problem**: Build fails due to missing dependencies
```
Solution: Check pe.md for required dependencies and install them:
- Boost (required)
- Eigen (usually header-only, may be bundled)
- CGAL (if PE_BUILD_CGAL=ON)
```

## Summary

To add PE to the superbuild:

1. **Add ExternalProject_Add** for PE in `third-party/CMakeLists.txt`
2. **Configure PE build options** (STATIC, no Irrlicht, optional CGAL)
3. **Add dependency** to `cmake/superbuild.cmake`
4. **Add option** for CGAL support in main `CMakeLists.txt`
5. **Find and link** PE in main project
6. **Install Boost** as prerequisite

The superbuild will then:
- Download PE from Git
- Configure with specified options
- Build PE library
- Install to `third-party/build/`
- Make PE available to your application

This provides a clean, reproducible integration of PE alongside bgfx, SDL, and ImGui.
