# Building and Running bgfx Examples

## Overview

The bgfx library comes with 40+ examples demonstrating various rendering techniques. When bgfx is integrated into this project via the superbuild system, the examples can optionally be built for learning and experimentation.

## Enabling bgfx Examples

By default, bgfx examples are **not built** to save compilation time. To enable them:

### Configure Superbuild with Examples

```bash
cmake -S . -B build -DSUPERBUILD=ON -DBGFX_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
```

**Configuration Options:**
- `SUPERBUILD=ON` - Use the superbuild system (required)
- `BGFX_BUILD_EXAMPLES=ON` - Enable building bgfx examples
- `CMAKE_BUILD_TYPE=Release` - Build in Release mode (faster, recommended for examples)

### Build Everything

```bash
cmake --build build --config Release -j$(nproc)
```

This will take longer than a regular build because it compiles all 40+ examples plus the shader compiler and supporting tools.

## Example Locations

After building, the example executables are located at:

```
third-party/build/src/bgfx-build/Release/cmake/bgfx/
```

**Example binaries:**
```
example-00-helloworld
example-01-cubes
example-02-metaballs
example-03-raymarch
example-04-mesh
... (40+ total examples)
```

## Running Examples

### Method 1: Change to bgfx Build Directory (Recommended)

The examples must be run from their build directory because they look for shaders and assets using relative paths.

```bash
cd third-party/build/src/bgfx-build/Release/cmake/bgfx/
./example-01-cubes
```

**Why this works:**
- Examples search for shaders in `../../../examples/runtime/shaders/`
- From the bgfx build directory, this path correctly resolves to the runtime assets

### Method 2: Run from Project Root (May Fail)

```bash
./third-party/build/src/bgfx-build/Release/cmake/bgfx/example-01-cubes
```

**This may fail with segfault or errors** because the example can't find the shader files when run from a different working directory.

## Shader Compilation

### Precompiled Shaders

bgfx examples come with **precompiled shaders** for multiple graphics APIs:

```
third-party/build/src/bgfx/bgfx/examples/runtime/shaders/
├── dx9/       # Direct3D 9 (Windows)
├── dx11/      # Direct3D 11 (Windows)
├── essl/      # OpenGL ES (mobile/web)
├── glsl/      # OpenGL (Linux, macOS)
├── metal/     # Metal (macOS, iOS)
├── pssl/      # PlayStation
└── spirv/     # Vulkan
```

bgfx automatically selects the correct shader format based on the active rendering backend.

### Shader Compiler (shaderc)

The shader compiler tool is also built:

```
third-party/build/src/bgfx-build/Release/cmake/bgfx/shaderc
```

**Shader source files** are `.sc` files (shader code):
```
third-party/build/src/bgfx/bgfx/examples/01-cubes/
├── cubes.cpp           # C++ code
├── vs_cubes.sc         # Vertex shader source
├── fs_cubes.sc         # Fragment shader source
├── varying.def.sc      # Vertex-to-fragment interface
└── makefile            # Shader compilation makefile
```

**To recompile shaders manually** (rarely needed, precompiled versions usually work):
```bash
cd third-party/build/src/bgfx/bgfx/examples/01-cubes/
make -C . SHADERC=../../.build/linux64_gcc/bin/shaderc
```

## Modifying and Rebuilding Examples

### Problem: Superbuild Doesn't Detect Changes

When you modify an example's source code (e.g., `cubes.cpp`), the top-level build command doesn't detect the change:

```bash
# This WON'T rebuild the modified example!
cmake --build build --config Release
```

**Why?** The examples are inside the bgfx ExternalProject. CMake treats external projects as opaque dependencies and doesn't track individual source files within them.

### Solution: Build Directly in bgfx Build Directory

```bash
# Rebuild a specific example
cmake --build third-party/build/src/bgfx-build/Release --target example-01-cubes -j3

# Or rebuild all examples
cmake --build third-party/build/src/bgfx-build/Release -j3
```

### Complete Workflow for Modifying Examples

1. **Edit the source file:**
   ```bash
   vim third-party/build/src/bgfx/bgfx/examples/01-cubes/cubes.cpp
   ```

2. **Rebuild the example:**
   ```bash
   cmake --build third-party/build/src/bgfx-build/Release --target example-01-cubes -j3
   ```

3. **Run the modified example:**
   ```bash
   cd third-party/build/src/bgfx-build/Release/cmake/bgfx/
   ./example-01-cubes
   ```

## Available Examples (Selected)

Here are the most useful examples for learning bgfx and PE integration:

### Fundamentals
- **00-helloworld** - Minimal setup, initializing bgfx
- **01-cubes** - Basic geometry rendering with vertex/index buffers
- **08-update** - Dynamic buffer updates (useful for animated geometry)

### Mesh and Geometry
- **04-mesh** - Loading and rendering 3D mesh files
- **05-instancing** - Efficient rendering of many identical objects
- **12-lod** - Level of detail management

### Debug and Visualization
- **28-wireframe** - Wireframe rendering techniques
- **29-debugdraw** - Debug drawing utilities (lines, boxes, spheres, text)
- **30-picking** - Mouse picking in 3D scenes

### Lighting and Shading
- **06-bump** - Normal/bump mapping
- **15-shadowmaps-simple** - Basic shadow mapping
- **16-shadowmaps** - Advanced shadow techniques

### Advanced Rendering
- **09-hdr** - High dynamic range rendering
- **18-ibl** - Image-based lighting
- **36-sky** - Procedural sky rendering

### Particles and Effects
- **32-particles** - GPU particle systems
- **38-bloom** - Bloom post-processing effect

## Example Controls

Most examples support common keyboard/mouse controls:

**Common Keys:**
- **F1** - Show/hide stats overlay
- **F3** - Toggle wireframe mode
- **ESC** - Quit application
- **Space** - Pause/resume (in some examples)

**Mouse:**
- **Left-click + drag** - Rotate camera (in examples with camera controls)
- **Right-click + drag** - Pan camera
- **Mouse wheel** - Zoom in/out

**Note:** Controls vary by example. Check the example's source code or on-screen instructions.

## ImGui Integration

All examples include Dear ImGui for runtime UI controls:

```cpp
imguiBeginFrame(/* mouse state */);
ImGui::Begin("Settings");
// ... ImGui widgets ...
ImGui::End();
imguiEndFrame();
```

**In 01-cubes:**
- Settings window (top-right corner)
- Checkboxes for R/G/B/A color channels
- Dropdown for primitive topology (Triangle List, Lines, Points, etc.)

## Debugging Examples

### If Example Crashes or Won't Start

1. **Check working directory:**
   ```bash
   # Must run from bgfx build directory
   cd third-party/build/src/bgfx-build/Release/cmake/bgfx/
   ./example-01-cubes
   ```

2. **Verify shaders exist:**
   ```bash
   ls -la third-party/build/src/bgfx/bgfx/examples/runtime/shaders/glsl/
   # Should see vs_cubes.bin, fs_cubes.bin, etc.
   ```

3. **Check graphics backend:**
   - On Linux, bgfx uses OpenGL by default
   - Ensure your system has working OpenGL drivers
   - For integrated Intel graphics, may need to update drivers

4. **Run with verbose output:**
   ```bash
   BGFX_CONFIG_DEBUG=1 ./example-01-cubes
   ```

### Common Issues

**Segfault on startup:**
- Running from wrong directory (shaders not found)
- Solution: `cd` to bgfx build directory

**Black screen:**
- Shader compilation/loading failed
- Check if precompiled shaders exist in runtime directory

**Window doesn't appear:**
- Display/X11 issues
- Check `$DISPLAY` environment variable
- Verify X server is running

## Build Performance

Building all examples takes significant time:

**Full superbuild with examples (first time):**
- ~5-10 minutes on modern hardware (depends on CPU cores)
- Uses shader compiler to compile shaders for all backends
- Builds 40+ example executables

**Incremental rebuild of single example:**
- ~2-5 seconds (just recompiling changed source)

**Tip:** Only enable `BGFX_BUILD_EXAMPLES=ON` when you need to study examples. Disable for faster iteration on your main project.

## Integration with Main Project

The examples are separate from your main `sdl-bgfx-imgui-starter` project. They share the bgfx library but have independent codebases.

**Learning workflow:**
1. Study example source code in `third-party/build/src/bgfx/bgfx/examples/`
2. Run the example to see it in action
3. Modify the example to experiment with concepts
4. Apply learned concepts to your main project (`main.cpp`)

**Example: Learning from 01-cubes for PE integration**
- Study how `cubes.cpp` creates vertex/index buffers
- Understand the rendering loop (setTransform → setVertexBuffer → submit)
- Experiment by modifying cube positions/colors
- Apply same pattern to render PE rigid bodies in `main.cpp`

## Quick Reference

### Build Examples
```bash
# Configure superbuild with examples enabled
cmake -S . -B build -DSUPERBUILD=ON -DBGFX_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release

# Build everything (includes examples)
cmake --build build --config Release -j$(nproc)
```

### Run Example
```bash
cd third-party/build/src/bgfx-build/Release/cmake/bgfx/
./example-01-cubes
```

### Modify and Rebuild Example
```bash
# Edit source
vim third-party/build/src/bgfx/bgfx/examples/01-cubes/cubes.cpp

# Rebuild example directly
cmake --build third-party/build/src/bgfx-build/Release --target example-01-cubes -j3

# Run modified example
cd third-party/build/src/bgfx-build/Release/cmake/bgfx/
./example-01-cubes
```

### Disable Examples (Faster Builds)
```bash
# Reconfigure without examples
cmake -S . -B build -DSUPERBUILD=ON -DBGFX_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release

# Rebuild (much faster)
cmake --build build --config Release -j$(nproc)
```

## Next Steps

1. **Run 01-cubes** - Familiarize yourself with the rendering
2. **Study the source** alongside the running example
3. **Modify the example** - Change colors, positions, camera
4. **Explore other examples** - 00-helloworld, 04-mesh, 29-debugdraw
5. **Apply to PE integration** - Use learned patterns in your main project

For detailed code analysis of specific examples, see:
- `doc/bgfx-01-cubes-study.md` - In-depth study of the cubes example
- `doc/bgfx-rendering-fundamentals.md` - Core rendering concepts
- `doc/pe-bgfx-integration-strategy.md` - How to apply examples to PE rendering
