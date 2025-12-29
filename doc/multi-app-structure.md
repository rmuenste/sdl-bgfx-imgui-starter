# Multi-Application Build Structure

## Overview

This project supports building multiple bgfx applications from a single codebase. The structure allows you to easily create new applications for experimentation and development without modifying the main executable.

## Directory Structure

```
bgfx-sdl-imgui/
├── main.cpp                      # Main starter executable
├── common/                       # Shared library code
│   ├── CMakeLists.txt           # Common library build
│   ├── sdl-imgui/               # ImGui SDL2 implementation
│   └── bgfx-imgui/              # ImGui bgfx implementation
├── examples/                     # Example applications
│   ├── CMakeLists.txt           # Auto-discovery of examples
│   └── 00-physics-basic/        # First example
│       ├── CMakeLists.txt       # Example build config
│       └── main.cpp             # Example source
├── cmake/
│   └── common-helpers.cmake     # Helper function for apps
└── CMakeLists.txt               # Root build configuration
```

## How It Works

### 1. Common Library (`common/`)

All shared code is compiled into a static library `bgfx-common`:
- ImGui SDL2 implementation
- ImGui bgfx implementation
- Debug draw functionality

This library is linked by all applications, so the code is compiled once and reused.

### 2. Helper Function (`add_bgfx_application()`)

The `cmake/common-helpers.cmake` file provides a helper function that handles all boilerplate:

```cmake
add_bgfx_application(
    NAME my-app
    SOURCES main.cpp other.cpp
)
```

This function automatically:
- Creates the executable target
- Links the common library
- Links all required dependencies (SDL2, bgfx, ImGui, PE, Boost)
- Sets up include directories
- Configures compiler flags (stop on first error)
- Sets up shader copying
- Configures platform-specific settings (Windows DLL, Emscripten, etc.)

### 3. Auto-Discovery of Examples

The `examples/CMakeLists.txt` automatically discovers and builds all example subdirectories that contain a `CMakeLists.txt` file.

## Creating a New Application

### Step 1: Create Directory

```bash
mkdir examples/01-my-example
```

### Step 2: Create CMakeLists.txt

```cmake
# examples/01-my-example/CMakeLists.txt

add_bgfx_application(
    NAME 01-my-example
    SOURCES main.cpp
)
```

That's it! Just two lines. All dependencies and configuration are handled automatically.

### Step 3: Create main.cpp

```cpp
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <debugdraw/debugdraw.h>
#include <SDL.h>
#include <SDL_syswm.h>

// PE headers (if needed)
#include <config.h>
#include <pe/core.h>

// ImGui headers
#include "bgfx-imgui/imgui_impl_bgfx.h"
#include "imgui.h"
#include "sdl-imgui/imgui_impl_sdl2.h"

int main(int argc, char** argv)
{
    // Your application code here
    // See examples/00-physics-basic/main.cpp for a complete example
}
```

### Step 4: Build

```bash
cmake --build build --config Release -j$(nproc)
```

The new example will be automatically discovered and built.

### Step 5: Run

```bash
./build/examples/01-my-example/01-my-example
```

## Available Headers

Because your application links to `bgfx-common` and the helper function sets up all includes, you have access to:

### bgfx and bx
```cpp
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
```

### Debug Draw
```cpp
#include <debugdraw/debugdraw.h>
```

### SDL2
```cpp
#include <SDL.h>
#include <SDL_syswm.h>
```

### ImGui
```cpp
#include "imgui.h"
#include "sdl-imgui/imgui_impl_sdl2.h"
#include "bgfx-imgui/imgui_impl_bgfx.h"
```

### PE Physics Engine
```cpp
#include <config.h>        // Must be first!
#include <pe/core.h>
#include <pe/core/Types.h>
```

## Multiple Source Files

If your application has multiple source files:

```cmake
add_bgfx_application(
    NAME 01-my-example
    SOURCES
        main.cpp
        physics_scene.cpp
        renderer.cpp
        utils.cpp
)
```

## Building Individual Applications

### Build everything:
```bash
cmake --build build --config Release -j$(nproc)
```

### Build just the main executable:
```bash
cmake --build build --target sdl-bgfx-imgui-starter --config Release
```

### Build just one example:
```bash
cmake --build build --target 00-physics-basic --config Release
```

## Example: 00-physics-basic

This example demonstrates:
- Setting up a PE physics world
- Creating spheres and a ground plane
- Stepping physics simulation at 60Hz
- Rendering bodies with debug draw
- Color-coding bodies with a palette
- Drawing orientation axes for each sphere
- ImGui UI for runtime info
- Orbit camera with mouse controls

**Run it:**
```bash
./build/examples/00-physics-basic/00-physics-basic
```

**Key features:**
- 3 spheres with different sizes falling
- Ground plane at Y=0
- Color palette (Green, Red, Blue for the 3 spheres)
- RGB axes showing sphere rotation
- FPS counter and body count in ImGui window
- Mouse drag to rotate camera

## Tips and Best Practices

### Naming Convention

Use two-digit prefixes for ordering:
- `00-physics-basic` - Minimal physics example
- `01-custom-materials` - Material system example
- `02-collision-shapes` - Different shape types
- etc.

### Keep Examples Focused

Each example should demonstrate one concept clearly. Don't try to pack too much into a single example.

### Copy and Modify

The easiest way to create a new example is to copy an existing one:

```bash
cp -r examples/00-physics-basic examples/01-my-example
# Edit examples/01-my-example/CMakeLists.txt to change NAME
# Edit examples/01-my-example/main.cpp
```

### Shared Code Between Examples

If multiple examples share code, you have two options:

**Option 1: Add to common library**
```cmake
# In common/CMakeLists.txt
target_sources(bgfx-common PRIVATE
    # ... existing sources ...
    shared/my_utility.cpp
)
```

**Option 2: Create a header-only utility**
```cpp
// examples/shared/my_utility.h
#pragma once
// ... inline functions ...
```

Then include it: `#include "../shared/my_utility.h"`

## Troubleshooting

### Example not building

Make sure you have a `CMakeLists.txt` in your example directory. The auto-discovery only finds directories with this file.

### Missing headers

All common headers should be available. If you're missing something, check that:
1. The header is provided by bgfx-common
2. You've included the correct path (use quotes for common/ headers)

### PE_PUBLIC undefined

Always include `<config.h>` before any PE headers:
```cpp
#include <config.h>  // FIRST!
#include <pe/core.h>
```

### Linker errors

The `add_bgfx_application()` helper handles all linking automatically. If you get linker errors, check:
1. You're using the helper function correctly
2. All source files are listed in SOURCES

## Architecture Benefits

This structure provides:

✅ **DRY (Don't Repeat Yourself)** - Common code compiled once, shared by all apps
✅ **Fast iteration** - Create new apps with just 2 lines of CMake
✅ **Consistent configuration** - All apps use same compiler flags, include paths
✅ **Easy maintenance** - Update dependencies in one place
✅ **Clean separation** - Main app separate from experimental examples
✅ **Scalable** - Add as many examples as needed without cluttering main code

## Migration from Old Structure

The old structure had all code in `main.cpp` with everything configured inline. The new structure:

**Before:**
- All configuration in root CMakeLists.txt (100+ lines)
- Creating new app = copy all boilerplate
- Shared code duplicated or messy includes

**After:**
- Configuration in helper function (reusable)
- Creating new app = 2 lines of CMake + main.cpp
- Shared code in common library

The main executable (`sdl-bgfx-imgui-starter`) now uses the same helper function, so it gets all the same benefits and consistency.
