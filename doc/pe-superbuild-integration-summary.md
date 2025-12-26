# PE Superbuild Integration - Implementation Summary

This document summarizes the changes made to integrate PE (Physics Engine) into the bgfx-sdl-imgui superbuild.

## Changes Made

### 1. Main CMakeLists.txt

**Added PE Build Option** (Line 18):
```cmake
option(PE_BUILD_CGAL "Build PE with CGAL support for triangle meshes" ON)
```

**Added PE Manual Linking** (Lines 37-63):
```cmake
# PE doesn't export CMake config, so we set paths manually
set(PE_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)
set(PE_LIBRARY_DIR ${CMAKE_PREFIX_PATH}/lib)
find_library(PE_LIBRARY NAMES pe libpe PATHS ${PE_LIBRARY_DIR} NO_DEFAULT_PATH)

# Add PE include directory
if (PE_INCLUDE_DIR)
  target_include_directories(${PROJECT_NAME} PRIVATE ${PE_INCLUDE_DIR})
endif()

# Link PE library if found
if (PE_LIBRARY)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${PE_LIBRARY})
  # PE also needs Boost
  find_package(Boost 1.46.1 REQUIRED COMPONENTS thread system filesystem program_options)
  target_link_libraries(${PROJECT_NAME} PRIVATE Boost::thread Boost::system Boost::filesystem Boost::program_options)
endif()
```

### 2. third-party/CMakeLists.txt

**Added Boost Check** (Lines 79-84):
```cmake
# PE requires Boost - check that it's available
find_package(Boost 1.46.1 REQUIRED COMPONENTS thread system filesystem program_options)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "PE requires Boost >= 1.46.1. Please install Boost first.")
endif()
```

**Added CGAL Check** (Lines 86-93):
```cmake
# Check for CGAL if enabled
if (PE_BUILD_CGAL)
  find_package(CGAL QUIET)
  if (NOT CGAL_FOUND)
    message(WARNING "CGAL not found. PE will be built without CGAL support.")
    set(PE_BUILD_CGAL OFF)
  endif()
endif()
```

**Added PE ExternalProject** (Lines 95-123):
```cmake
ExternalProject_Add(
  pe
  GIT_REPOSITORY https://github.com/rmuenste/pe-0.3rc1.git
  GIT_TAG main
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
             -DCGAL=${PE_BUILD_CGAL}
             -DUSE_JSON=ON
             -DMPI=OFF
             -DOPENCL=OFF
             -DBLAS=OFF
  BUILD_COMMAND cmake --build <BINARY_DIR> ${build_config_arg}
  # PE doesn't have install targets, so we manually copy files
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
                  <SOURCE_DIR>/pe
                  <INSTALL_DIR>/include/pe
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  <BINARY_DIR>/libpe_static.a
                  <INSTALL_DIR>/lib/libpe.a
          COMMAND ${CMAKE_COMMAND} -E echo "PE installed to <INSTALL_DIR>"
)
```

### 3. cmake/superbuild.cmake

**Added PE to Dependencies** (Line 1):
```cmake
list(APPEND THIRD_PARTY_DEPENDENCIES bgfx imgui.cmake pe)
```

## How It Works

### Build Flow

1. **Superbuild Phase** (when `SUPERBUILD=ON`):
   ```
   CMake configures superbuild
   ├── Checks for Boost (required for PE)
   ├── Checks for CGAL (optional, controlled by PE_BUILD_CGAL)
   ├── Downloads SDL2, bgfx, imgui.cmake, pe from Git
   ├── Builds each in order (respecting dependencies)
   └── Installs to third-party/build/
       ├── include/pe/  (PE headers)
       └── lib/libpe.a  (PE static library)
   ```

2. **Main Project Build** (when `SUPERBUILD=OFF`):
   ```
   CMake configures main project
   ├── Finds SDL2, bgfx, imgui via find_package()
   ├── Manually locates PE library and headers
   ├── Links everything together
   └── Builds the executable
   ```

### PE Installation Workaround

Since PE doesn't have CMake install targets, we manually copy:
- **Headers**: `<SOURCE_DIR>/pe/` → `<INSTALL_DIR>/include/pe/`
- **Library**: `<BINARY_DIR>/libpe_static.a` → `<INSTALL_DIR>/lib/libpe.a`

This makes PE available via standard paths that CMake can find.

## Build Commands

### Prerequisites

Boost must be installed on your system:

**Ubuntu/Debian**:
```bash
sudo apt-get install libboost-all-dev
```

**macOS**:
```bash
brew install boost
```

**Windows (vcpkg)**:
```cmd
vcpkg install boost-thread boost-system boost-filesystem boost-program-options
```

### Basic Build (No CGAL)

```bash
cd /home/rmuenste/code/bgfx-sdl-imgui
mkdir build
cd build

# Configure with superbuild
cmake -DSUPERBUILD=ON -DCMAKE_BUILD_TYPE=Release -DPE_BUILD_CGAL=OFF ..

# Build everything
cmake --build .

# Run
./sdl-bgfx-imgui-starter
```

### Build with CGAL Support

First install CGAL:
```bash
# Ubuntu/Debian
sudo apt-get install libcgal-dev

# macOS
brew install cgal
```

Then build:
```bash
cd /home/rmuenste/code/bgfx-sdl-imgui
mkdir build
cd build

cmake -DSUPERBUILD=ON -DCMAKE_BUILD_TYPE=Release -DPE_BUILD_CGAL=ON ..
cmake --build .
```

### Clean Rebuild

```bash
# Remove build directory
rm -rf build/

# Also remove cached third-party builds if needed
rm -rf third-party/build/

# Start fresh
mkdir build && cd build
cmake -DSUPERBUILD=ON ..
cmake --build .
```

## Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `SUPERBUILD` | OFF | Enable superbuild mode |
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug/Release) |
| `BGFX_BUILD_EXAMPLES` | OFF (Release) / ON (Debug) | Build bgfx examples |
| `PE_BUILD_CGAL` | ON | Build PE with CGAL support |

## File Structure After Build

```
bgfx-sdl-imgui/
├── build/
│   ├── sdl-bgfx-imgui-starter    # Your executable
│   └── shader/                    # Shader files (auto-copied)
└── third-party/
    └── build/
        ├── include/
        │   ├── SDL2/
        │   ├── bgfx/
        │   ├── bx/
        │   ├── bimg/
        │   ├── imgui.h
        │   └── pe/               # ← PE headers
        ├── lib/
        │   ├── libSDL2.a
        │   ├── libbgfx.a
        │   ├── libimgui.a
        │   └── libpe.a           # ← PE library
        └── src/
            ├── SDL2/             # Downloaded source
            ├── bgfx/
            ├── imgui.cmake/
            └── pe/               # ← PE source
```

## Troubleshooting

### Problem: "Could NOT find Boost"

**Solution**: Install Boost or set `BOOST_ROOT`:
```bash
export BOOST_ROOT=/usr/local
cmake -DSUPERBUILD=ON ..
```

### Problem: "CGAL not found" Warning

**Solution**: Either install CGAL or disable it:
```bash
cmake -DSUPERBUILD=ON -DPE_BUILD_CGAL=OFF ..
```

### Problem: "libpe_static.a not found" During Install

**Cause**: PE build output might be named differently (e.g., Windows: `pe_static.lib`)

**Solution**: Check actual library name:
```bash
ls third-party/build/src/pe-build/Release/
```

Then update `third-party/CMakeLists.txt` INSTALL_COMMAND to match:
```cmake
COMMAND ${CMAKE_COMMAND} -E copy_if_different
        <BINARY_DIR>/pe_static.lib      # ← Adjust this
        <INSTALL_DIR>/lib/pe.lib
```

### Problem: Build fails with "pe.h: No such file"

**Cause**: Headers not copied correctly

**Solution**: Check that `<SOURCE_DIR>/pe` exists:
```bash
ls third-party/build/src/pe/pe/
```

If headers are in a different location, update INSTALL_COMMAND.

### Problem: Linker errors about undefined Boost symbols

**Cause**: PE needs Boost, but it's not linked

**Solution**: The main CMakeLists.txt should automatically find and link Boost when PE is found. Verify Boost is installed.

## Next Steps

Now that PE is integrated:

1. **Test the build**: Run the build commands above
2. **Verify PE is available**: Check that headers and library are installed
3. **Use PE in code**: Include PE headers and create physics simulations
4. **Integrate with bgfx**: Follow the integration strategy in `pe-bgfx-integration-strategy.md`

## Example: Using PE in main.cpp

Once built, you can use PE in your application:

```cpp
#include <pe/pe.h>
#include <bgfx/bgfx.h>

// In your initialization:
pe::WorldID world = pe::createWorld();
pe::BoxID box = pe::createBox(1, pe::Vec3(0, 5, 0), pe::Vec3(1, 1, 1), pe::oak);

// In your update loop:
world->simulateStep(1.0f / 60.0f);

// Get position for rendering:
const pe::Vec3& pos = box->getPosition();
const pe::Quat& rot = box->getRotation();
// ... convert to bgfx transform and render ...
```

## Summary

The integration required:
- **3 files modified**: CMakeLists.txt, third-party/CMakeLists.txt, cmake/superbuild.cmake
- **~60 lines added** total
- **1 prerequisite**: Boost (must be installed)
- **1 optional dependency**: CGAL (for triangle mesh support)

PE is now part of the superbuild and will be automatically:
- Downloaded from https://github.com/rmuenste/pe-0.3rc1.git
- Configured with static library build
- Built with Irrlicht disabled (using bgfx instead)
- Installed to `third-party/build/`
- Linked into the main executable
