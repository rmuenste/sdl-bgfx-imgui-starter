# bgfx 01-cubes Example Study

## Overview

The `01-cubes` example demonstrates the core geometry rendering pipeline in bgfx. This is the foundational knowledge needed to render physics objects from the PE library.

**What This Example Teaches:**
1. How to define and create vertex/index buffers for geometry
2. How to set up view and projection matrices for 3D rendering
3. How to render multiple instances of the same geometry
4. How to control render states (depth testing, culling, color channels)
5. Different primitive topology types (triangles, lines, points)

## Key Concepts

### 1. Vertex Data Structure

```cpp
struct PosColorVertex
{
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_abgr;  // Color in ABGR format (Alpha, Blue, Green, Red)

    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();
    };

    static bgfx::VertexLayout ms_layout;
};
```

**Why This Matters:**
- Defines the structure of each vertex in memory
- `bgfx::VertexLayout` tells bgfx how to interpret the vertex data
- The layout is used when creating vertex buffers
- PE rigid bodies (Box, Sphere, etc.) will need similar vertex structures

**Color Format:**
- `0xAABBGGRR` - packed 32-bit ABGR
- Example: `0xff0000ff` = opaque red (A=255, B=0, G=0, R=255)
- The `true` parameter in `.add()` means the Uint8 values are normalized to [0,1]

### 2. Cube Geometry Data

#### Vertex Array (8 corners of cube)
```cpp
static PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 },  // 0: front-top-left (black)
    { 1.0f,  1.0f,  1.0f, 0xff0000ff },  // 1: front-top-right (red)
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 },  // 2: front-bottom-left (green)
    { 1.0f, -1.0f,  1.0f, 0xff00ffff },  // 3: front-bottom-right (yellow)
    {-1.0f,  1.0f, -1.0f, 0xffff0000 },  // 4: back-top-left (blue)
    { 1.0f,  1.0f, -1.0f, 0xffff00ff },  // 5: back-top-right (magenta)
    {-1.0f, -1.0f, -1.0f, 0xffffff00 },  // 6: back-bottom-left (cyan)
    { 1.0f, -1.0f, -1.0f, 0xffffffff },  // 7: back-bottom-right (white)
};
```

**Vertex Reuse:**
- Only 8 vertices needed for a cube
- Index buffers specify which vertices form each triangle
- This is more memory-efficient than duplicating vertices

#### Triangle List Indices (12 triangles = 6 faces × 2 triangles/face)
```cpp
static const uint16_t s_cubeTriList[] =
{
    0, 1, 2,  // Front face - triangle 1
    1, 3, 2,  // Front face - triangle 2
    4, 6, 5,  // Back face - triangle 1
    5, 6, 7,  // Back face - triangle 2
    0, 2, 4,  // Left face - triangle 1
    4, 2, 6,  // Left face - triangle 2
    1, 5, 3,  // Right face - triangle 1
    5, 7, 3,  // Right face - triangle 2
    0, 4, 1,  // Top face - triangle 1
    4, 5, 1,  // Top face - triangle 2
    2, 3, 6,  // Bottom face - triangle 1
    6, 3, 7,  // Bottom face - triangle 2
};
```

**Winding Order:**
- Counter-clockwise winding when viewed from outside
- Important for backface culling (BGFX_STATE_CULL_CW)
- CW = clockwise faces are culled (back faces)

### 3. Alternative Primitive Topologies

The example demonstrates 5 different ways to render the same cube:

1. **Triangle List** - Default, most common
   - Each group of 3 indices forms a triangle
   - 36 indices for 12 triangles

2. **Triangle Strip** - More memory efficient
   - First triangle uses indices 0,1,2
   - Each subsequent index adds a new triangle
   - Indices alternate winding order automatically

3. **Line List** - Wireframe rendering (edges only)
   - Each pair of indices forms a line segment
   - 24 indices for 12 edges

4. **Line Strip** - Connected wireframe
   - Creates continuous connected lines
   - More compact than line list

5. **Points** - Just the 8 vertices
   - Useful for debugging vertex positions

**PE Relevance:**
- Physics bodies will primarily use triangle lists for solid rendering
- Line lists useful for debug visualization (collision shapes, contact normals)
- Points useful for visualizing particle systems

### 4. Buffer Creation (in init())

```cpp
// Create vertex stream declaration
PosColorVertex::init();

// Create static vertex buffer
m_vbh = bgfx::createVertexBuffer(
    bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)),
    PosColorVertex::ms_layout
);

// Create static index buffer for triangle list rendering
m_ibh[0] = bgfx::createIndexBuffer(
    bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList))
);
```

**Key Points:**
- `bgfx::makeRef()` wraps static data without copying (zero-copy reference)
- For dynamic data, use `bgfx::copy()` to let bgfx manage the memory
- Vertex layout must match the vertex structure exactly
- Buffers persist across frames (created once in init, used in every update)

**Static vs Dynamic:**
- Static: Data won't change (cube vertices) - use `makeRef()`
- Dynamic: Data updates per frame (PE body positions) - use `copy()` or transient buffers

### 5. Camera and View Setup

```cpp
const bx::Vec3 at  = { 0.0f, 0.0f,   0.0f };  // Look-at point (origin)
const bx::Vec3 eye = { 0.0f, 0.0f, -35.0f };  // Camera position (back on Z)

float view[16];
bx::mtxLookAt(view, eye, at);

float proj[16];
bx::mtxProj(proj, 60.0f, float(m_width)/float(m_height), 0.1f, 100.0f,
    bgfx::getCaps()->homogeneousDepth);

bgfx::setViewTransform(0, view, proj);
bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
```

**View Matrix (Camera Transform):**
- `bx::mtxLookAt()` creates view matrix from eye position and look-at target
- Standard "look-at" camera positioning
- For PE: could create orbit camera or free-flying camera

**Projection Matrix:**
- `bx::mtxProj()` creates perspective projection
- Parameters: FOV (60°), aspect ratio, near plane (0.1), far plane (100.0)
- `homogeneousDepth` - handles different NDC depth ranges (OpenGL [−1,1] vs D3D/Vulkan/Metal [0,1])

**View 0:**
- Every frame we set the view transform for view 0
- All cubes are submitted to view 0
- Views are independent rendering passes

### 6. Rendering Loop - The Core Pattern

```cpp
// Submit 11x11 cubes
for (uint32_t yy = 0; yy < 11; ++yy)
{
    for (uint32_t xx = 0; xx < 11; ++xx)
    {
        // 1. Calculate model matrix (position + rotation)
        float mtx[16];
        bx::mtxRotateXY(mtx, time + xx*0.21f, time + yy*0.37f);
        mtx[12] = -15.0f + float(xx)*3.0f;  // X translation
        mtx[13] = -15.0f + float(yy)*3.0f;  // Y translation
        mtx[14] = 0.0f;                     // Z translation

        // 2. Set transform for this draw call
        bgfx::setTransform(mtx);

        // 3. Bind vertex and index buffers
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(ibh);

        // 4. Set render states
        bgfx::setState(state);

        // 5. Submit to view 0 with shader program
        bgfx::submit(0, m_program);
    }
}

// 6. Advance frame
bgfx::frame();
```

**The 6-Step Rendering Pattern:**

1. **Calculate Model Matrix**
   - Transforms object from local space to world space
   - Can combine rotation, scale, translation
   - Each cube gets unique position and rotation

2. **Set Transform** - `bgfx::setTransform(mtx)`
   - Associates this matrix with the next draw call
   - Transform is per-object, not per-view

3. **Bind Buffers**
   - `setVertexBuffer(slot, handle)` - vertex data
   - `setIndexBuffer(handle)` - index data
   - Same buffers can be reused for multiple instances

4. **Set Render States** - `bgfx::setState(state)`
   - Controls depth testing, blending, culling, etc.
   - More details in section 7 below

5. **Submit Draw Call** - `bgfx::submit(viewId, program)`
   - Queues this draw call for view 0
   - Doesn't render immediately - just records commands
   - All preceding `set*()` calls apply to this submit

6. **Advance Frame** - `bgfx::frame()`
   - Called once per frame (not per object!)
   - Processes all queued commands
   - Presents the frame to the screen

**For PE Integration:**
```cpp
// Pseudo-code for rendering PE rigid bodies
for (auto& body : world->getBodies()) {
    float mtx[16];
    body->getTransformMatrix(mtx);  // Get body's current pose

    bgfx::setTransform(mtx);
    bgfx::setVertexBuffer(0, body->getVertexBuffer());
    bgfx::setIndexBuffer(body->getIndexBuffer());
    bgfx::setState(state);
    bgfx::submit(0, m_program);
}
bgfx::frame();
```

### 7. Render States

```cpp
uint64_t state = 0
    | (m_r ? BGFX_STATE_WRITE_R : 0)      // Write red channel
    | (m_g ? BGFX_STATE_WRITE_G : 0)      // Write green channel
    | (m_b ? BGFX_STATE_WRITE_B : 0)      // Write blue channel
    | (m_a ? BGFX_STATE_WRITE_A : 0)      // Write alpha channel
    | BGFX_STATE_WRITE_Z                  // Write to depth buffer
    | BGFX_STATE_DEPTH_TEST_LESS          // Depth test: closer objects win
    | BGFX_STATE_CULL_CW                  // Cull clockwise (back) faces
    | BGFX_STATE_MSAA                     // Multisample anti-aliasing
    | s_ptState[m_pt]                     // Primitive topology state
    ;
```

**Common State Flags:**

- **Write Masks:**
  - `BGFX_STATE_WRITE_RGB` - Write all color channels
  - `BGFX_STATE_WRITE_A` - Write alpha channel
  - `BGFX_STATE_WRITE_Z` - Write depth buffer
  - Useful for effects (e.g., render shadows without color)

- **Depth Testing:**
  - `BGFX_STATE_DEPTH_TEST_LESS` - Standard 3D rendering
  - `BGFX_STATE_DEPTH_TEST_ALWAYS` - Always pass (no depth test)
  - `BGFX_STATE_DEPTH_TEST_GREATER` - Reverse depth buffer

- **Culling:**
  - `BGFX_STATE_CULL_CW` - Cull clockwise faces (most common)
  - `BGFX_STATE_CULL_CCW` - Cull counter-clockwise faces
  - No flag = no culling (render both sides)

- **Blending:**
  - Not shown here, but available via `BGFX_STATE_BLEND_*`
  - Important for transparency and particle effects

**For PE:** Standard opaque rendering would use:
```cpp
BGFX_STATE_WRITE_RGB
| BGFX_STATE_WRITE_Z
| BGFX_STATE_DEPTH_TEST_LESS
| BGFX_STATE_CULL_CW
| BGFX_STATE_MSAA
```

### 8. Matrix Math Helpers

The example uses `bx` library matrix utilities:

```cpp
// Rotation matrix
bx::mtxRotateXY(mtx, angleX, angleY);

// In-place translation (modifying matrix elements directly)
mtx[12] = x;  // Column 3, Row 0 - X translation
mtx[13] = y;  // Column 3, Row 1 - Y translation
mtx[14] = z;  // Column 3, Row 2 - Z translation
```

**Standard 4x4 Matrix Layout (Column-Major):**
```
[ 0  4  8  12 ]   [ Xx Yx Zx Tx ]
[ 1  5  9  13 ] = [ Xy Yy Zy Ty ]
[ 2  6 10  14 ]   [ Xz Yz Zz Tz ]
[ 3  7 11  15 ]   [ 0  0  0  1  ]
```
- Columns 0-2: Rotation/scale (X, Y, Z axes)
- Column 3: Translation (T)

**Other Useful bx Matrix Functions:**
- `bx::mtxIdentity(mtx)` - Identity matrix
- `bx::mtxTranslate(mtx, x, y, z)` - Pure translation
- `bx::mtxScale(mtx, x, y, z)` - Pure scale
- `bx::mtxRotateX/Y/Z(mtx, angle)` - Rotation around axis
- `bx::mtxMul(result, a, b)` - Matrix multiplication
- `bx::mtxInverse(result, mtx)` - Matrix inverse (for view matrix from camera transform)

**For PE:**
- PE provides rotation matrices via quaternions
- Convert PE's transform to 4x4 matrix for bgfx
- Or use `bx::mtxFromQuaternion()` if available

### 9. ImGui Integration

```cpp
imguiCreate();  // In init()

// In update()
imguiBeginFrame(/* mouse state */);

ImGui::Begin("Settings", NULL, 0);
ImGui::Checkbox("Write R", &m_r);
ImGui::Checkbox("Write G", &m_g);
ImGui::Checkbox("Write B", &m_b);
ImGui::Checkbox("Write A", &m_a);
ImGui::Text("Primitive topology:");
ImGui::Combo("##topology", (int*)&m_pt, s_ptNames, BX_COUNTOF(s_ptNames));
ImGui::End();

imguiEndFrame();

// ... rendering ...

bgfx::frame();

// In shutdown()
imguiDestroy();
```

**ImGui Order:**
1. `imguiBeginFrame()` - Start ImGui frame, pass mouse/window state
2. ImGui widget calls (`Begin`, `Checkbox`, `Combo`, etc.)
3. `imguiEndFrame()` - Finish ImGui, generates draw commands
4. Render 3D geometry
5. `bgfx::frame()` - Submit everything

**For PE:** Can add runtime controls:
- Time scale slider
- Gravity vector
- Solver iterations
- Visualization toggles (wireframe, contact points, velocity vectors)

### 10. Time and Animation

```cpp
m_timeOffset = bx::getHPCounter();  // In init()

// In update()
float time = (float)((bx::getHPCounter() - m_timeOffset) /
                      double(bx::getHPFrequency()));
```

**High-Precision Timer:**
- `bx::getHPCounter()` - Current high-precision timestamp
- `bx::getHPFrequency()` - Ticks per second
- Result: `time` in seconds since initialization

**Animation:**
- Each cube rotates based on `time + xx*0.21f` and `time + yy*0.37f`
- Different multipliers create varied rotation speeds
- For PE: use physics delta time, not just wall clock time

### 11. Shaders

```cpp
m_program = loadProgram("vs_cubes", "fs_cubes");
```

**Shader Program:**
- Combines vertex shader (`vs_cubes`) and fragment shader (`fs_cubes`)
- Must be compiled for target platform (D3D, OpenGL, Metal, Vulkan, etc.)
- bgfx's `shaderc` tool compiles shaders from a common format

**Typical Vertex Shader (vs_cubes):**
```glsl
// Inputs: vertex position, color
// Outputs: transformed position, interpolated color
// Uses: model, view, projection matrices
```

**Typical Fragment Shader (fs_cubes):**
```glsl
// Inputs: interpolated color
// Outputs: final pixel color
```

**For PE:** Will need custom shaders for:
- Basic shading (diffuse + specular)
- Normal mapping
- Shadow mapping
- Debug visualization (color-coded velocities, forces)

### 12. Resource Cleanup

```cpp
int shutdown() override
{
    imguiDestroy();

    // Cleanup buffers
    for (uint32_t ii = 0; ii < BX_COUNTOF(m_ibh); ++ii) {
        bgfx::destroy(m_ibh[ii]);
    }
    bgfx::destroy(m_vbh);
    bgfx::destroy(m_program);

    // Shutdown bgfx
    bgfx::shutdown();

    return 0;
}
```

**Proper Cleanup Order:**
1. Destroy ImGui
2. Destroy all index buffers
3. Destroy vertex buffers
4. Destroy shader programs
5. Shutdown bgfx

**Important:**
- All bgfx resources must be destroyed before `bgfx::shutdown()`
- Handles are lightweight (just IDs), actual cleanup happens in `destroy()`

## PE Integration Takeaways

### What You Need to Render PE Bodies:

1. **For Each Body Type (Box, Sphere, Cylinder, etc.):**
   - Vertex array with positions (and normals for lighting)
   - Index array defining triangles
   - Vertex layout describing the structure
   - Create vertex/index buffers once

2. **Each Frame:**
   - Query PE body transforms (position + orientation)
   - Convert to 4x4 matrices
   - For each visible body:
     - Set transform
     - Bind buffers
     - Set state
     - Submit

3. **Camera:**
   - Orbit camera tracking center of simulation
   - Or free camera for exploration
   - Update view matrix based on user input

4. **Shaders:**
   - Basic vertex shader: transform position, pass normals
   - Basic fragment shader: simple lighting (diffuse + ambient)
   - Later: shadows, better materials

### Rendering Pattern Comparison

**01-cubes (static cubes with animation):**
```cpp
// One vertex/index buffer pair
// 11×11 instances with different transforms
for (int y = 0; y < 11; ++y) {
    for (int x = 0; x < 11; ++x) {
        setTransform(calculateTransform(x, y, time));
        setVertexBuffer(cubeVBH);
        setIndexBuffer(cubeIBH);
        setState(state);
        submit(view, program);
    }
}
```

**PE Integration (dynamic rigid bodies):**
```cpp
// Multiple vertex/index buffer pairs (one per body type)
// N instances based on simulation state
for (auto& body : world->getRigidBodies()) {
    setTransform(body->getWorldTransform());
    setVertexBuffer(body->getGeometry()->getVertexBuffer());
    setIndexBuffer(body->getGeometry()->getIndexBuffer());
    setState(state);
    submit(view, program);
}
```

**Key Difference:**
- Cubes: transforms calculated from math formulas
- PE: transforms come from physics simulation state

## Next Steps

1. **Study the shaders** (`shader/vs_cubes.sc`, `shader/fs_cubes.sc`)
   - Understand vertex transformation
   - Color interpolation

2. **Modify the example:**
   - Change cube colors
   - Add scaling animation
   - Implement orbit camera with mouse

3. **Start PE integration:**
   - Create simple Box geometry in PE format
   - Render a single static box
   - Add PE physics and render dynamic boxes
   - Expand to other primitives (Sphere, Capsule)

4. **Study example 03-raymarch or 06-bump:**
   - More advanced shading techniques
   - Normal mapping for better visuals

## Quick Reference

### Essential bgfx Calls for Geometry Rendering

```cpp
// Initialization
PosColorVertex::init();
vbh = bgfx::createVertexBuffer(bgfx::makeRef(verts, size), layout);
ibh = bgfx::createIndexBuffer(bgfx::makeRef(indices, size));
program = loadProgram("vs_shader", "fs_shader");

// Per-frame setup
bgfx::setViewClear(viewId, flags, color, depth, stencil);
bgfx::setViewRect(viewId, x, y, width, height);
bgfx::setViewTransform(viewId, view, proj);

// Per-object rendering
bgfx::setTransform(modelMatrix);
bgfx::setVertexBuffer(0, vbh);
bgfx::setIndexBuffer(ibh);
bgfx::setState(state);
bgfx::submit(viewId, program);

// Frame end
bgfx::frame();

// Cleanup
bgfx::destroy(vbh);
bgfx::destroy(ibh);
bgfx::destroy(program);
bgfx::shutdown();
```

### Common Render States

```cpp
// Opaque 3D geometry (most common for PE)
BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z
| BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA

// Transparent geometry (particles, glass)
BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_LESS
| BGFX_STATE_BLEND_ALPHA | BGFX_STATE_CULL_CW

// Wireframe debug rendering
BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_LESS
| BGFX_STATE_PT_LINES
```

### Matrix Construction

```cpp
// Identity
float mtx[16];
bx::mtxIdentity(mtx);

// Translation only
bx::mtxTranslate(mtx, x, y, z);

// Rotation + translation
float rot[16], trans[16], result[16];
bx::mtxRotateXYZ(rot, pitch, yaw, roll);
bx::mtxTranslate(trans, x, y, z);
bx::mtxMul(result, rot, trans);  // result = trans * rot

// From quaternion (useful for PE)
bx::mtxFromQuaternion(mtx, quat);
mtx[12] = x; mtx[13] = y; mtx[14] = z;  // Add translation
```
