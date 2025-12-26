# PE-bgfx Integration Strategy

This document outlines the strategy for integrating bgfx as a modern, future-proof rendering backend for the PE (Physics Engine) rigid body dynamics simulation library.

## Table of Contents
- [Motivation](#motivation)
- [Current PE Visualization Options](#current-pe-visualization-options)
- [Why bgfx as an Alternative](#why-bgfx-as-an-alternative)
- [Integration Architecture](#integration-architecture)
- [Implementation Strategy](#implementation-strategy)
- [Learning Path](#learning-path)
- [Coordinate System Mapping](#coordinate-system-mapping)
- [Rendering PE Primitives](#rendering-pe-primitives)
- [Debug Visualization Features](#debug-visualization-features)
- [Performance Considerations](#performance-considerations)

## Motivation

### The Irrlicht Problem

The PE library currently uses Irrlicht for real-time 3D visualization. However:

- **Last commit**: 12 years ago (abandoned)
- **API Support**: Limited to OpenGL and DirectX 9 (both legacy)
- **Modern Features**: No support for modern rendering techniques
- **Future-proofing**: Strong chance of becoming completely legacy soon
- **Platform Support**: Limited mobile/web support

**Conclusion**: Irrlicht is not future-proof. A modern alternative is needed.

### Why Not Other Options?

Current PE visualization options and their limitations:

| Option | Type | Issues |
|--------|------|--------|
| **Irrlicht** | Real-time | Abandoned, legacy APIs only |
| **POV-Ray** | Offline | Not real-time, export-based |
| **VTK** | Scientific | Heavy, primarily for post-processing |
| **OpenDX** | Scientific | Specialized, not general-purpose |

**Need**: A modern, actively maintained, real-time 3D renderer.

## Current PE Visualization Options

**Location**: See `pe.md` Architecture Overview

### pe/irrlicht Module

```cpp
// Current Irrlicht integration pattern
#include <pe/irrlicht.h>

pe::WorldID world = pe::createWorld();
pe::irrlicht::ViewerID viewer = pe::irrlicht::createViewer();

// Add bodies to physics world
pe::BoxID box = pe::createBox(...);

// Visualize in Irrlicht
viewer->add(box);

// Main loop
while (running) {
    world->simulateStep(dt);
    viewer->render();
}
```

**Problem**: This pattern is tied to Irrlicht's scene graph architecture.

### pe/povray Module

Export-based workflow for high-quality offline rendering. Not suitable for interactive simulation.

## Why bgfx as an Alternative

### Advantages Over Irrlicht

| Feature | Irrlicht | bgfx |
|---------|----------|------|
| **Last Update** | 2013 (12 years ago) | Active (2024) |
| **Graphics APIs** | OpenGL, D3D9 | D3D11/12, Vulkan, Metal, GL, WebGL |
| **Mobile Support** | Limited | iOS, Android native |
| **Web Support** | None | WebGL/WebGPU via Emscripten |
| **Modern Pipeline** | Fixed function | Modern shader-based |
| **Multi-threading** | Single-threaded | Multi-threaded renderer |
| **Abstraction Level** | Scene graph (high) | Command submission (low) |
| **File Size** | ~10MB library | ~2MB core |
| **Maintenance** | Abandoned | Active development |
| **Community** | Stagnant | Active (used in production) |

### bgfx Philosophy

- **Low-level control** with high-level API abstraction
- **Platform-agnostic** rendering (write once, run everywhere)
- **Performance-focused** with multi-threaded architecture
- **Modern techniques** without legacy baggage
- **Minimal dependencies** (unlike heavy engines)

### Real-World Usage

bgfx is used in production by:
- Game engines (custom engines, not Unity/Unreal)
- CAD/visualization software
- Scientific simulation visualization
- Embedded systems with 3D requirements

## Integration Architecture

### High-Level Design

```
┌─────────────────────────────────────────────────────┐
│                 Application Layer                    │
│  (Physics Simulation + Visualization Control)        │
└─────────────┬───────────────────────────────────────┘
              │
    ┌─────────┴─────────┐
    │                   │
┌───▼────────┐    ┌────▼─────────────────────────┐
│ PE Library │    │  pe/bgfx Visualization       │
│  (Physics) │    │  (New Module)                │
└───┬────────┘    └────┬─────────────────────────┘
    │                  │
    │  Query State     │  Submit Draw Calls
    │  (positions,     │
    │   rotations,     │
    │   bodies)        │
    │                  │
    └──────────────────┼──────────────┐
                       │              │
                 ┌─────▼──────┐  ┌───▼────────┐
                 │   bgfx     │  │   SDL      │
                 │  (Render)  │  │  (Window)  │
                 └────────────┘  └────────────┘
```

### Module Structure

Create a new module: `pe/bgfx/`

```
pe/
├── bgfx/
│   ├── Viewer.h           # Main viewer class (like irrlicht::ViewerID)
│   ├── Config.h           # bgfx-specific configuration
│   ├── MeshBuilder.h      # Geometry creation utilities
│   ├── Camera.h           # Camera control
│   └── DebugDraw.h        # Debug visualization helpers
├── core/
│   └── (existing PE core)
└── irrlicht/
    └── (deprecated, kept for compatibility)
```

### Core Classes

#### 1. Viewer Class

```cpp
// pe/bgfx/Viewer.h
namespace pe {
namespace bgfx {

class Viewer : public entry::AppI
{
public:
    Viewer(const char* name, const char* description);

    // Add PE bodies to visualization
    void add(RigidBodyID body);
    void remove(RigidBodyID body);

    // Camera control
    void setCamera(const Vec3& position, const Vec3& target);
    void setCameraFollowing(RigidBodyID body);

    // Debug visualization
    void enableContactPoints(bool enable);
    void enableContactNormals(bool enable);
    void enableBoundingBoxes(bool enable);
    void enableVelocityVectors(bool enable);

    // Rendering configuration
    void setBackgroundColor(float r, float g, float b);
    void enableShadows(bool enable);
    void setLightDirection(const Vec3& direction);

    // AppI interface
    void init(int32_t argc, const char* const* argv, uint32_t width, uint32_t height) override;
    bool update() override;
    int shutdown() override;

private:
    void renderBodies();
    void renderDebugInfo();
    void updateCamera();

    std::vector<RigidBodyID> m_bodies;
    Camera m_camera;

    // bgfx resources
    bgfx::ProgramHandle m_meshShader;
    bgfx::ProgramHandle m_debugShader;

    // Geometry for different primitive types
    bgfx::VertexBufferHandle m_boxVB;
    bgfx::IndexBufferHandle m_boxIB;
    bgfx::VertexBufferHandle m_sphereVB;
    bgfx::IndexBufferHandle m_sphereIB;
    // ... other primitives

    // Configuration
    bool m_showContacts;
    bool m_showNormals;
    bool m_showBounds;
    bool m_showVelocities;
    bool m_shadowsEnabled;
};

typedef Viewer* ViewerID;

// Factory function (matches PE pattern)
ViewerID createViewer(const char* name = "PE Physics Viewer",
                      const char* description = "Real-time physics visualization");

} // namespace bgfx
} // namespace pe
```

#### 2. Camera Class

```cpp
// pe/bgfx/Camera.h
namespace pe {
namespace bgfx {

class Camera
{
public:
    Camera();

    // Camera control
    void setPosition(const Vec3& pos);
    void setTarget(const Vec3& target);
    void orbit(float deltaAzimuth, float deltaElevation);
    void zoom(float delta);
    void pan(float deltaX, float deltaY);

    // Following mode
    void follow(RigidBodyID body);
    void stopFollowing();

    // Update (called per frame)
    void update(float deltaTime);

    // Get matrices for rendering
    void getViewMatrix(float* view) const;
    void getProjectionMatrix(float* proj, float aspect) const;

private:
    Vec3 m_position;
    Vec3 m_target;
    Vec3 m_up;

    float m_distance;
    float m_azimuth;
    float m_elevation;

    RigidBodyID m_followBody;
    Vec3 m_followOffset;
};

} // namespace bgfx
} // namespace pe
```

#### 3. MeshBuilder Utility

```cpp
// pe/bgfx/MeshBuilder.h
namespace pe {
namespace bgfx {

class MeshBuilder
{
public:
    // Create geometry for PE primitives
    static void createBox(float width, float height, float depth,
                         bgfx::VertexBufferHandle& vbh,
                         bgfx::IndexBufferHandle& ibh);

    static void createSphere(float radius, uint32_t segments,
                            bgfx::VertexBufferHandle& vbh,
                            bgfx::IndexBufferHandle& ibh);

    static void createCapsule(float radius, float length, uint32_t segments,
                             bgfx::VertexBufferHandle& vbh,
                             bgfx::IndexBufferHandle& ibh);

    static void createCylinder(float radius, float length, uint32_t segments,
                              bgfx::VertexBufferHandle& vbh,
                              bgfx::IndexBufferHandle& ibh);

    static void createPlane(float width, float depth, uint32_t tesselation,
                           bgfx::VertexBufferHandle& vbh,
                           bgfx::IndexBufferHandle& ibh);

    // Create mesh from PE TriangleMesh
    static void createFromTriangleMesh(TriangleMeshID mesh,
                                      bgfx::VertexBufferHandle& vbh,
                                      bgfx::IndexBufferHandle& ibh);

    // Debug geometry
    static void createContactPoint(float size,
                                   bgfx::VertexBufferHandle& vbh,
                                   bgfx::IndexBufferHandle& ibh);

    static void createArrow(float length, float headSize,
                           bgfx::VertexBufferHandle& vbh,
                           bgfx::IndexBufferHandle& ibh);
};

} // namespace bgfx
} // namespace pe
```

## Implementation Strategy

### Phase 1: Minimal Integration (HelloWorld Level)

**Goal**: Get a window open with PE physics running, debug text showing stats.

```cpp
// examples/bgfx_minimal/main.cpp
#include <pe/pe.h>
#include <pe/bgfx/Viewer.h>

class MinimalPhysicsDemo : public pe::bgfx::Viewer
{
public:
    MinimalPhysicsDemo()
        : Viewer("Minimal Physics", "Basic PE + bgfx integration")
    {}

    void init(int32_t argc, const char* const* argv, uint32_t width, uint32_t height) override
    {
        // Initialize bgfx
        Viewer::init(argc, argv, width, height);

        // Initialize PE world
        m_world = pe::createWorld();

        // Add a simple ground plane
        pe::PlaneID ground = pe::createPlane(0, Vec3(0, 1, 0), 0, pe::granite);

        // Add a box above ground
        pe::BoxID box = pe::createBox(1, Vec3(0, 5, 0), Vec3(1, 1, 1), pe::oak);

        // Register bodies with viewer
        add(ground);
        add(box);
    }

    bool update() override
    {
        // Run physics step
        m_world->simulateStep(1.0f / 60.0f);

        // Render (handled by base Viewer class)
        return Viewer::update();
    }

    int shutdown() override
    {
        // Cleanup handled by base class
        return Viewer::shutdown();
    }

private:
    pe::WorldID m_world;
};

ENTRY_IMPLEMENT_MAIN(MinimalPhysicsDemo, "minimal-physics", "Minimal PE + bgfx demo");
```

**Deliverables**:
- Dark gray window opens
- Debug text shows: "Physics bodies: 2"
- Physics simulation runs (but nothing visible yet)

### Phase 2: Basic Geometry Rendering

**Goal**: Render Box and Sphere primitives with simple shading.

**Tasks**:
1. Implement `MeshBuilder::createBox()` and `MeshBuilder::createSphere()`
2. Create basic vertex/fragment shaders (Phong shading)
3. Implement `Viewer::renderBodies()` to iterate and render each body
4. Extract transform from PE bodies and convert to 4x4 matrices

**Code Pattern**:
```cpp
void Viewer::renderBodies()
{
    for (RigidBodyID body : m_bodies)
    {
        // Get transform from PE
        const Vec3& pos = body->getPosition();
        const Quat& rot = body->getRotation();

        // Convert to bgfx matrix
        float mtx[16];
        quatToMatrix(mtx, pos, rot);

        // Set transform
        bgfx::setTransform(mtx);

        // Get geometry based on body type
        auto [vb, ib] = getGeometryForBody(body);

        // Set buffers and state
        bgfx::setVertexBuffer(0, vb);
        bgfx::setIndexBuffer(ib);
        bgfx::setState(BGFX_STATE_DEFAULT);

        // Submit draw call
        bgfx::submit(0, m_meshShader);
    }
}
```

**Deliverables**:
- Box and sphere visible on screen
- Bodies fall due to gravity
- Simple diffuse shading

### Phase 3: All Primitive Types

**Goal**: Support all PE geometry primitives.

**Primitives to implement**:
- [x] Box
- [x] Sphere
- [ ] Capsule
- [ ] Cylinder
- [ ] Plane (infinite plane as large quad)
- [ ] TriangleMesh (from .obj files)
- [ ] Union (composite of primitives)

**Deliverables**:
- All PE primitive types can be visualized
- Different examples for each type

### Phase 4: Camera Control

**Goal**: Interactive camera for scene inspection.

**Features**:
- Orbit camera (mouse drag)
- Zoom (mouse wheel)
- Pan (middle mouse button)
- Follow mode (track specific body)
- Keyboard shortcuts (reset, top view, side view, etc.)

**Deliverables**:
- Fully controllable camera
- Smooth camera movements
- Multiple camera presets

### Phase 5: Lighting & Materials

**Goal**: Better visual quality with proper lighting.

**Features**:
- Directional light (sun)
- Multiple material types (wood, metal, plastic, glass)
- Phong or PBR shading
- Basic shadows (shadow mapping)

**Deliverables**:
- Realistic lighting
- Material properties from PE (friction coefficient → shininess)
- Contact shadows for depth perception

### Phase 6: Debug Visualization

**Goal**: Visualize physics debugging information.

**Features**:
- **Contact points**: Small red spheres at collision points
- **Contact normals**: Green arrows showing normal direction
- **Velocity vectors**: Blue arrows showing body velocities
- **Bounding boxes**: Wireframe AABBs
- **Angular velocity**: Curved arrows showing rotation
- **Force vectors**: Yellow arrows showing applied forces

**Implementation**:
```cpp
void Viewer::renderDebugInfo()
{
    if (m_showContacts)
    {
        for (auto contact : m_world->getContacts())
        {
            // Draw small sphere at contact point
            renderSphere(contact.position, 0.05f, Color::Red);

            if (m_showNormals)
            {
                // Draw normal as arrow
                renderArrow(contact.position,
                           contact.position + contact.normal * 0.5f,
                           Color::Green);
            }
        }
    }

    if (m_showVelocities)
    {
        for (RigidBodyID body : m_bodies)
        {
            Vec3 pos = body->getPosition();
            Vec3 vel = body->getLinearVelocity();
            renderArrow(pos, pos + vel, Color::Blue);
        }
    }

    // ... similar for other debug features
}
```

**Deliverables**:
- Toggleable debug overlays
- Visual confirmation of physics correctness
- Useful for debugging collision issues

### Phase 7: TriangleMesh Support

**Goal**: Render complex triangle meshes (critical for PE's CGAL integration).

**Tasks**:
1. Load .obj files using existing PE mesh loading
2. Convert PE TriangleMesh to bgfx vertex/index buffers
3. Support for DistanceMap-accelerated meshes
4. Proper normal calculation for smooth shading

**Code Pattern**:
```cpp
void MeshBuilder::createFromTriangleMesh(TriangleMeshID mesh,
                                        bgfx::VertexBufferHandle& vbh,
                                        bgfx::IndexBufferHandle& ibh)
{
    // Get mesh data from PE
    const auto& vertices = mesh->getVertices();
    const auto& triangles = mesh->getTriangles();

    // Convert to bgfx format
    std::vector<PosNormalVertex> verts;
    std::vector<uint16_t> indices;

    for (const auto& tri : triangles)
    {
        // Calculate normals, convert vertices
        // ...
    }

    // Create bgfx buffers
    const bgfx::Memory* vbMem = bgfx::copy(verts.data(), verts.size() * sizeof(PosNormalVertex));
    const bgfx::Memory* ibMem = bgfx::copy(indices.data(), indices.size() * sizeof(uint16_t));

    vbh = bgfx::createVertexBuffer(vbMem, PosNormalVertex::ms_layout);
    ibh = bgfx::createIndexBuffer(ibMem);
}
```

**Deliverables**:
- Complex meshes render correctly
- Integration with PE's CGAL examples
- DistanceMap collision visualization

### Phase 8: Performance Optimization

**Goal**: Handle large simulations efficiently.

**Optimizations**:
- Instanced rendering for repeated geometries
- Frustum culling (don't render off-screen bodies)
- Level-of-detail (LOD) for distant objects
- Batching static bodies
- Occlusion culling for dense scenes

**Deliverables**:
- Smooth 60 FPS with 1000+ bodies
- Efficient memory usage
- Profiling tools integration

## Learning Path

To implement this integration, follow this learning sequence:

### 1. Current: 00-helloworld ✓
**Status**: Completed

**Learned**:
- bgfx initialization
- The init/update/shutdown lifecycle
- Views and clearing
- Debug text rendering
- The `bgfx::frame()` concept

### 2. Next: 01-cubes
**Goal**: Understand geometry rendering

**Will learn**:
- Creating vertex/index buffers
- Loading and compiling shaders
- Setting transforms (model matrices)
- Basic rendering pipeline
- Render states

**PE Relevance**: Rendering Box primitives

### 3. Then: 04-mesh
**Goal**: Load and render triangle meshes

**Will learn**:
- Loading .obj files
- Vertex formats with normals
- Index buffer optimization
- Smooth vs flat shading

**PE Relevance**: Rendering TriangleMesh bodies (critical for CGAL integration)

### 4. Then: 11-fontsdf or 10-font
**Goal**: Render text for debug info

**Will learn**:
- Font rendering techniques
- Text layout
- HUD/UI overlay

**PE Relevance**: Displaying simulation statistics, body info

### 5. Then: 16-shadowmaps
**Goal**: Add shadows for depth perception

**Will learn**:
- Multi-pass rendering
- Shadow mapping technique
- Framebuffer usage
- Depth rendering

**PE Relevance**: Better visualization of contact points and body stacking

### 6. Then: 06-bump (optional)
**Goal**: Advanced shading techniques

**Will learn**:
- Normal mapping
- Texture sampling
- Advanced material properties

**PE Relevance**: More realistic materials for different body types

### 7. Finally: Custom PE Integration
**Goal**: Complete pe/bgfx module

**Tasks**:
- Implement all phases from Implementation Strategy
- Create example applications
- Write documentation and tutorials
- Performance testing with large simulations

## Coordinate System Mapping

### PE Coordinate System

PE uses a right-handed coordinate system:
- **X**: Right
- **Y**: Up
- **Z**: Forward (out of screen)

### bgfx Coordinate System

bgfx is API-agnostic and handles coordinate system conversions internally. However, you typically work in:
- **X**: Right
- **Y**: Up
- **Z**: Forward (OpenGL convention) or Backward (DirectX convention)

bgfx handles the conversion based on the selected backend.

### Transform Conversion

Converting PE rigid body transforms to bgfx matrices:

```cpp
void quatToMatrix(float* mtx, const pe::Vec3& position, const pe::Quat& rotation)
{
    // PE uses quaternion for rotation: (w, x, y, z)
    float w = rotation.w;
    float x = rotation.x;
    float y = rotation.y;
    float z = rotation.z;

    // Convert to 4x4 column-major matrix for bgfx
    mtx[0] = 1.0f - 2.0f * (y*y + z*z);
    mtx[1] = 2.0f * (x*y + w*z);
    mtx[2] = 2.0f * (x*z - w*y);
    mtx[3] = 0.0f;

    mtx[4] = 2.0f * (x*y - w*z);
    mtx[5] = 1.0f - 2.0f * (x*x + z*z);
    mtx[6] = 2.0f * (y*z + w*x);
    mtx[7] = 0.0f;

    mtx[8] = 2.0f * (x*z + w*y);
    mtx[9] = 2.0f * (y*z - w*x);
    mtx[10] = 1.0f - 2.0f * (x*x + y*y);
    mtx[11] = 0.0f;

    mtx[12] = position.x;
    mtx[13] = position.y;
    mtx[14] = position.z;
    mtx[15] = 1.0f;
}
```

Alternatively, use bx (bgfx's math library):

```cpp
#include <bx/math.h>

void peTransformToBgfx(float* mtx, const pe::Vec3& pos, const pe::Quat& rot)
{
    bx::Quaternion q = { rot.x, rot.y, rot.z, rot.w };
    float rotMtx[16];
    bx::mtxFromQuaternion(rotMtx, q);

    // Apply translation
    rotMtx[12] = pos.x;
    rotMtx[13] = pos.y;
    rotMtx[14] = pos.z;

    bx::memCopy(mtx, rotMtx, sizeof(float) * 16);
}
```

## Rendering PE Primitives

### Vertex Format

Define a standard vertex format for PE rendering:

```cpp
struct PosNormalVertex
{
    float m_x, m_y, m_z;        // Position
    uint32_t m_normal;          // Packed normal

    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true)
            .end();
    }

    static bgfx::VertexLayout ms_layout;
};
```

### Box Primitive

```cpp
void MeshBuilder::createBox(float width, float height, float depth,
                           bgfx::VertexBufferHandle& vbh,
                           bgfx::IndexBufferHandle& ibh)
{
    // Box has 8 vertices, but need 24 for proper normals (6 faces × 4 corners)
    PosNormalVertex vertices[24];

    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float hd = depth * 0.5f;

    // Front face (+Z)
    vertices[0] = { -hw, -hh,  hd, encodeNormal(0, 0, 1) };
    vertices[1] = {  hw, -hh,  hd, encodeNormal(0, 0, 1) };
    vertices[2] = {  hw,  hh,  hd, encodeNormal(0, 0, 1) };
    vertices[3] = { -hw,  hh,  hd, encodeNormal(0, 0, 1) };

    // Back face (-Z)
    vertices[4] = {  hw, -hh, -hd, encodeNormal(0, 0, -1) };
    // ... continue for all 6 faces ...

    // Indices (2 triangles per face, 6 faces)
    uint16_t indices[36] = {
        0, 1, 2,  0, 2, 3,    // Front
        4, 5, 6,  4, 6, 7,    // Back
        // ... continue for all faces ...
    };

    vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices, sizeof(vertices)),
        PosNormalVertex::ms_layout
    );

    ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(indices, sizeof(indices))
    );
}
```

### Sphere Primitive

```cpp
void MeshBuilder::createSphere(float radius, uint32_t segments,
                              bgfx::VertexBufferHandle& vbh,
                              bgfx::IndexBufferHandle& ibh)
{
    std::vector<PosNormalVertex> vertices;
    std::vector<uint16_t> indices;

    // UV sphere generation
    for (uint32_t lat = 0; lat <= segments; ++lat)
    {
        float theta = lat * M_PI / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (uint32_t lon = 0; lon <= segments; ++lon)
        {
            float phi = lon * 2.0f * M_PI / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            PosNormalVertex v;
            v.m_x = radius * sinTheta * cosPhi;
            v.m_y = radius * cosTheta;
            v.m_z = radius * sinTheta * sinPhi;

            // Normal is same as position for unit sphere
            float nx = sinTheta * cosPhi;
            float ny = cosTheta;
            float nz = sinTheta * sinPhi;
            v.m_normal = encodeNormal(nx, ny, nz);

            vertices.push_back(v);
        }
    }

    // Generate indices
    for (uint32_t lat = 0; lat < segments; ++lat)
    {
        for (uint32_t lon = 0; lon < segments; ++lon)
        {
            uint16_t first = lat * (segments + 1) + lon;
            uint16_t second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices.data(), vertices.size() * sizeof(PosNormalVertex)),
        PosNormalVertex::ms_layout
    );

    ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(indices.data(), indices.size() * sizeof(uint16_t))
    );
}
```

### Rendering by Type

```cpp
void Viewer::renderBody(RigidBodyID body)
{
    // Get transform
    float mtx[16];
    peTransformToBgfx(mtx, body->getPosition(), body->getRotation());
    bgfx::setTransform(mtx);

    // Get geometry based on type
    GeometryType type = body->getType();

    switch (type)
    {
    case BOX:
    {
        auto box = static_cast<BoxID>(body);
        // Could use pre-built unit box and scale in matrix
        // Or build box with exact dimensions
        bgfx::setVertexBuffer(0, m_boxVB);
        bgfx::setIndexBuffer(m_boxIB);
        break;
    }

    case SPHERE:
    {
        auto sphere = static_cast<SphereID>(body);
        bgfx::setVertexBuffer(0, m_sphereVB);
        bgfx::setIndexBuffer(m_sphereIB);
        break;
    }

    case CAPSULE:
        bgfx::setVertexBuffer(0, m_capsuleVB);
        bgfx::setIndexBuffer(m_capsuleIB);
        break;

    case CYLINDER:
        bgfx::setVertexBuffer(0, m_cylinderVB);
        bgfx::setIndexBuffer(m_cylinderIB);
        break;

    case PLANE:
        // Render as large quad
        bgfx::setVertexBuffer(0, m_planeVB);
        bgfx::setIndexBuffer(m_planeIB);
        break;

    case TRIANGLEMESH:
    {
        auto mesh = static_cast<TriangleMeshID>(body);
        // Each mesh has unique geometry
        auto [vb, ib] = getMeshGeometry(mesh);
        bgfx::setVertexBuffer(0, vb);
        bgfx::setIndexBuffer(ib);
        break;
    }

    default:
        return; // Unknown type
    }

    // Set material properties (could be uniforms)
    // Set render state
    bgfx::setState(BGFX_STATE_DEFAULT);

    // Submit
    bgfx::submit(0, m_meshShader);
}
```

## Debug Visualization Features

### Contact Point Visualization

```cpp
void Viewer::renderContactPoints()
{
    if (!m_showContacts) return;

    // Get contacts from PE world
    const auto& contacts = m_world->getContacts();

    for (const auto& contact : contacts)
    {
        // Draw small sphere at contact point
        float mtx[16];
        bx::mtxTranslate(mtx, contact.position.x, contact.position.y, contact.position.z);

        float scale[16];
        bx::mtxScale(scale, 0.05f, 0.05f, 0.05f);

        float transform[16];
        bx::mtxMul(transform, scale, mtx);

        bgfx::setTransform(transform);
        bgfx::setVertexBuffer(0, m_contactPointVB);
        bgfx::setIndexBuffer(m_contactPointIB);

        // Set red color uniform
        float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        bgfx::setUniform(m_colorUniform, color);

        bgfx::setState(BGFX_STATE_DEFAULT);
        bgfx::submit(0, m_debugShader);
    }
}
```

### Velocity Vector Visualization

```cpp
void Viewer::renderVelocityVectors()
{
    if (!m_showVelocities) return;

    for (RigidBodyID body : m_bodies)
    {
        Vec3 pos = body->getPosition();
        Vec3 vel = body->getLinearVelocity();

        float speed = vel.length();
        if (speed < 0.01f) continue; // Skip stationary bodies

        // Draw arrow from position in direction of velocity
        drawArrow(pos, pos + vel, Color(0, 0, 1, 1));
    }
}

void Viewer::drawArrow(const Vec3& from, const Vec3& to, const Color& color)
{
    // Line part
    PosColorVertex lineVerts[2] = {
        { from.x, from.y, from.z, color.rgba },
        { to.x, to.y, to.z, color.rgba }
    };

    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, 2, PosColorVertex::ms_layout);
    bx::memCopy(tvb.data, lineVerts, sizeof(lineVerts));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(BGFX_STATE_PT_LINES | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::submit(0, m_lineShader);

    // Arrow head (small cone at 'to' position)
    // ... render cone geometry ...
}
```

### Bounding Box Visualization

```cpp
void Viewer::renderBoundingBoxes()
{
    if (!m_showBounds) return;

    for (RigidBodyID body : m_bodies)
    {
        // Get AABB from PE
        Vec3 min = body->getAABB().min();
        Vec3 max = body->getAABB().max();

        // Draw wireframe box
        drawWireBox(min, max, Color(0, 1, 0, 1));
    }
}

void Viewer::drawWireBox(const Vec3& min, const Vec3& max, const Color& color)
{
    // 12 lines for box edges
    Vec3 corners[8] = {
        { min.x, min.y, min.z },
        { max.x, min.y, min.z },
        { max.x, max.y, min.z },
        { min.x, max.y, min.z },
        { min.x, min.y, max.z },
        { max.x, min.y, max.z },
        { max.x, max.y, max.z },
        { min.x, max.y, max.z }
    };

    uint16_t lineIndices[24] = {
        // Bottom face
        0, 1,  1, 2,  2, 3,  3, 0,
        // Top face
        4, 5,  5, 6,  6, 7,  7, 4,
        // Vertical edges
        0, 4,  1, 5,  2, 6,  3, 7
    };

    // Create and submit line geometry
    // ...
}
```

## Performance Considerations

### Instanced Rendering

For scenes with many identical bodies (e.g., particle simulations, box stacks):

```cpp
void Viewer::renderBodiesInstanced()
{
    // Group bodies by type
    std::map<GeometryType, std::vector<RigidBodyID>> bodyGroups;
    for (auto body : m_bodies)
    {
        bodyGroups[body->getType()].push_back(body);
    }

    // Render each group with instancing
    for (const auto& [type, bodies] : bodyGroups)
    {
        if (bodies.empty()) continue;

        // Collect instance transforms
        std::vector<float> instanceTransforms;
        instanceTransforms.reserve(bodies.size() * 16);

        for (auto body : bodies)
        {
            float mtx[16];
            peTransformToBgfx(mtx, body->getPosition(), body->getRotation());
            instanceTransforms.insert(instanceTransforms.end(), mtx, mtx + 16);
        }

        // Create instance buffer
        const bgfx::Memory* mem = bgfx::copy(
            instanceTransforms.data(),
            instanceTransforms.size() * sizeof(float)
        );

        bgfx::InstanceDataBuffer idb;
        bgfx::allocInstanceDataBuffer(&idb, bodies.size(), 64); // 64 bytes per instance (4x4 matrix)
        bx::memCopy(idb.data, mem->data, mem->size);

        // Set geometry
        auto [vb, ib] = getGeometryForType(type);
        bgfx::setVertexBuffer(0, vb);
        bgfx::setIndexBuffer(ib);
        bgfx::setInstanceDataBuffer(&idb);

        // Submit instanced draw
        bgfx::setState(BGFX_STATE_DEFAULT);
        bgfx::submit(0, m_meshShader);
    }
}
```

**Performance gain**: 100x speedup for 10,000 identical boxes.

### Frustum Culling

Don't render bodies outside the camera view:

```cpp
bool Viewer::isInFrustum(RigidBodyID body)
{
    // Extract frustum planes from view-projection matrix
    float viewProj[16];
    bx::mtxMul(viewProj, m_viewMatrix, m_projMatrix);

    Plane frustumPlanes[6];
    extractFrustumPlanes(frustumPlanes, viewProj);

    // Get body bounding sphere
    Vec3 center = body->getPosition();
    float radius = body->getBoundingSphereRadius();

    // Test against all 6 planes
    for (int i = 0; i < 6; ++i)
    {
        if (frustumPlanes[i].distance(center) < -radius)
        {
            return false; // Outside this plane
        }
    }

    return true; // Inside frustum
}

void Viewer::renderBodiesCulled()
{
    uint32_t visible = 0;
    uint32_t culled = 0;

    for (auto body : m_bodies)
    {
        if (isInFrustum(body))
        {
            renderBody(body);
            ++visible;
        }
        else
        {
            ++culled;
        }
    }

    // Display stats
    bgfx::dbgTextPrintf(0, 1, 0x0f, "Visible: %d  Culled: %d", visible, culled);
}
```

### Level of Detail (LOD)

Use simpler geometry for distant objects:

```cpp
void Viewer::renderBodyWithLOD(RigidBodyID body)
{
    float distance = (body->getPosition() - m_camera.getPosition()).length();

    // Choose LOD based on distance
    LODLevel lod;
    if (distance < 10.0f)
        lod = LOD_HIGH;
    else if (distance < 50.0f)
        lod = LOD_MEDIUM;
    else
        lod = LOD_LOW;

    // Get appropriate geometry
    auto [vb, ib] = getGeometryForBodyLOD(body, lod);

    // Render as normal
    // ...
}
```

**Example LOD levels**:
- **HIGH**: Sphere with 32 segments (2048 triangles)
- **MEDIUM**: Sphere with 16 segments (512 triangles)
- **LOW**: Sphere with 8 segments (128 triangles)

### Static Body Batching

Bodies that don't move can be merged into a single draw call:

```cpp
void Viewer::buildStaticGeometryBatch()
{
    std::vector<PosNormalVertex> batchVertices;
    std::vector<uint16_t> batchIndices;

    for (auto body : m_bodies)
    {
        if (!body->isFixed()) continue; // Skip dynamic bodies

        // Get geometry and transform
        auto [vb, ib] = getGeometryForBody(body);
        float mtx[16];
        peTransformToBgfx(mtx, body->getPosition(), body->getRotation());

        // Transform vertices and add to batch
        // ... merge into batchVertices and batchIndices ...
    }

    // Create single merged mesh
    m_staticBatchVB = bgfx::createVertexBuffer(...);
    m_staticBatchIB = bgfx::createIndexBuffer(...);
}

void Viewer::renderStaticBatch()
{
    // One draw call for all static geometry!
    bgfx::setVertexBuffer(0, m_staticBatchVB);
    bgfx::setIndexBuffer(m_staticBatchIB);
    bgfx::setState(BGFX_STATE_DEFAULT);
    bgfx::submit(0, m_meshShader);
}
```

## Summary

This integration strategy provides:

1. **Modern rendering** replacing the abandoned Irrlicht engine
2. **Future-proof** with active development and cross-platform support
3. **Flexible architecture** allowing both simple and advanced visualizations
4. **Performance** for large-scale simulations
5. **Debug tools** for physics development
6. **Clean integration** following PE's existing patterns

The phased implementation approach allows gradual development while maintaining working code at each stage. Starting from basic debug text (Phase 1) and progressing to full-featured visualization with shadows and debug overlays (Phases 6-8).

**Next steps**:
1. Study bgfx example 01-cubes for geometry rendering
2. Implement Phase 1: Minimal Integration
3. Test with existing PE examples (boxstack, etc.)
4. Gradually add features from subsequent phases
