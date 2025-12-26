# PE Debug Draw Quick Start

## Overview

The bgfx `debugdraw` library (from example-29) provides a simple, immediate-mode API for rendering primitive shapes. This is **perfect** for quickly visualizing PE physics simulations without writing custom vertex buffers and shaders.

**Why Start with Debug Draw:**
- ✅ No manual vertex/index buffer creation
- ✅ No shader compilation needed
- ✅ Immediate-mode API (easy to use)
- ✅ Built-in support for PE's primitive types (Sphere, Capsule, Cylinder, Box)
- ✅ Get visualization working in minutes, not hours

**Later:** Once the simulation is working, you can replace debug draw with optimized rendering using vertex buffers (as shown in 01-cubes).

## Debug Draw API Basics

### Include and Setup

```cpp
#include <debugdraw/debugdraw.h>

// In init()
ddInit();  // Initialize debug draw system

// In shutdown()
ddShutdown();  // Cleanup debug draw
```

### Per-Frame Rendering Pattern

```cpp
// In your render loop (update())
DebugDrawEncoder dde;

dde.begin(viewId);  // Start encoding debug draw commands for view 0

// Set rendering state
dde.setColor(0xffffffff);      // White color (ABGR format)
dde.setWireframe(false);       // Solid rendering (or true for wireframe)

// Draw primitives
bx::Sphere sphere = { { 0.0f, 5.0f, 0.0f }, 1.0f };  // center, radius
dde.draw(sphere);

dde.end();  // Finish encoding

bgfx::frame();  // Submit frame as usual
```

## PE Primitive to bx Geometry Mapping

PE has several rigid body primitive types. Here's how to render each using debug draw:

### 1. Sphere

**PE Class:** `pe::Sphere`
**Debug Draw:** `bx::Sphere`

```cpp
// PE Sphere API
inline const Vec3& Sphere::getPosition() const;   // Center position
inline real        Sphere::getRadius() const;     // Radius

// Convert to debug draw
pe::SphereID sphere = /* get from PE world */;

bx::Sphere bxSphere;
bxSphere.center.x = sphere->getPosition()[0];
bxSphere.center.y = sphere->getPosition()[1];
bxSphere.center.z = sphere->getPosition()[2];
bxSphere.radius   = sphere->getRadius();

dde.draw(bxSphere);
```

### 2. Capsule

**PE Class:** `pe::Capsule`
**Debug Draw:** `bx::Capsule`

```cpp
// PE Capsule API
inline const Vec3& Capsule::getPosition() const;  // Center position
inline real        Capsule::getRadius() const;    // Radius
inline real        Capsule::getLength() const;    // Length along axis
inline const Rot3& Capsule::getRotation() const;  // Orientation

// Convert to debug draw
pe::CapsuleID capsule = /* get from PE world */;

// Get capsule axis from rotation matrix
const pe::Rot3& rot = capsule->getRotation();
pe::Vec3 axis = rot[1];  // Y-axis in local space (capsule is aligned with Y)

// Calculate endpoints
float halfLength = capsule->getLength() * 0.5f;
pe::Vec3 pos = capsule->getPosition();
pe::Vec3 start = pos - axis * halfLength;
pe::Vec3 end   = pos + axis * halfLength;

bx::Capsule bxCapsule;
bxCapsule.pos.x = start[0];
bxCapsule.pos.y = start[1];
bxCapsule.pos.z = start[2];
bxCapsule.end.x = end[0];
bxCapsule.end.y = end[1];
bxCapsule.end.z = end[2];
bxCapsule.radius = capsule->getRadius();

dde.draw(bxCapsule);
```

### 3. Box

**PE Class:** `pe::Box`
**Debug Draw:** `bx::Aabb` (axis-aligned) or `bx::Obb` (oriented)

**Important:** PE boxes are **oriented** (have rotation), so use `bx::Obb`.

```cpp
// PE Box API
inline const Vec3& Box::getPosition() const;     // Center position
inline const Vec3& Box::getLengths() const;      // Dimensions (full width/height/depth)
inline const Rot3& Box::getRotation() const;     // Orientation matrix

// Convert to debug draw
pe::BoxID box = /* get from PE world */;

bx::Obb bxObb;

// Copy rotation and position into 4x4 matrix
const pe::Rot3& rot = box->getRotation();
const pe::Vec3& pos = box->getPosition();
const pe::Vec3& lengths = box->getLengths();

// Set rotation part (3x3 upper-left, scaled by half-extents)
float sx = lengths[0] * 0.5f;  // Half-width
float sy = lengths[1] * 0.5f;  // Half-height
float sz = lengths[2] * 0.5f;  // Half-depth

// Column-major 4x4 matrix
bxObb.mtx[ 0] = rot[0][0] * sx;  bxObb.mtx[ 4] = rot[1][0] * sy;  bxObb.mtx[ 8] = rot[2][0] * sz;  bxObb.mtx[12] = pos[0];
bxObb.mtx[ 1] = rot[0][1] * sx;  bxObb.mtx[ 5] = rot[1][1] * sy;  bxObb.mtx[ 9] = rot[2][1] * sz;  bxObb.mtx[13] = pos[1];
bxObb.mtx[ 2] = rot[0][2] * sx;  bxObb.mtx[ 6] = rot[1][2] * sy;  bxObb.mtx[10] = rot[2][2] * sz;  bxObb.mtx[14] = pos[2];
bxObb.mtx[ 3] = 0.0f;            bxObb.mtx[ 7] = 0.0f;            bxObb.mtx[11] = 0.0f;            bxObb.mtx[15] = 1.0f;

dde.draw(bxObb);
```

**Simpler Alternative for Box (using setTransform):**

```cpp
float mtx[16];
// Build transform matrix from PE rotation + position
// ... (convert PE Rot3 + Vec3 to 4x4 matrix)

dde.push();  // Save state
dde.setTransform(mtx);  // Apply transform

// Draw unit box at origin (will be transformed)
bx::Aabb unitBox;
unitBox.min = { -0.5f * lengths[0], -0.5f * lengths[1], -0.5f * lengths[2] };
unitBox.max = {  0.5f * lengths[0],  0.5f * lengths[1],  0.5f * lengths[2] };
dde.draw(unitBox);

dde.pop();  // Restore state
```

### 4. Plane

**PE Class:** `pe::Plane`
**Debug Draw:** `dde.drawGrid()`

```cpp
// PE Plane API
inline const Vec3& Plane::getNormal() const;     // Plane normal
inline real        Plane::getDisplacement() const; // Distance from origin

// Convert to debug draw
pe::PlaneID plane = /* get from PE world */;

pe::Vec3 normal = plane->getNormal();
float d = plane->getDisplacement();

// Calculate point on plane
pe::Vec3 center = normal * d;

bx::Vec3 bxNormal = { normal[0], normal[1], normal[2] };
bx::Vec3 bxCenter = { center[0], center[1], center[2] };

dde.drawGrid(bxNormal, bxCenter, 20, 1.0f);  // 20x20 grid, 1.0 spacing
```

### 5. Cylinder

**PE Class:** `pe::Cylinder`
**Debug Draw:** `bx::Cylinder`

```cpp
// PE Cylinder API (similar to Capsule)
inline const Vec3& Cylinder::getPosition() const;
inline real        Cylinder::getRadius() const;
inline real        Cylinder::getLength() const;
inline const Rot3& Cylinder::getRotation() const;

// Convert (same pattern as Capsule)
pe::CylinderID cylinder = /* get from PE world */;

const pe::Rot3& rot = cylinder->getRotation();
pe::Vec3 axis = rot[1];  // Y-axis

float halfLength = cylinder->getLength() * 0.5f;
pe::Vec3 pos = cylinder->getPosition();
pe::Vec3 start = pos - axis * halfLength;
pe::Vec3 end   = pos + axis * halfLength;

bx::Cylinder bxCylinder;
bxCylinder.pos.x = start[0];
bxCylinder.pos.y = start[1];
bxCylinder.pos.z = start[2];
bxCylinder.end.x = end[0];
bxCylinder.end.y = end[1];
bxCylinder.end.z = end[2];
bxCylinder.radius = cylinder->getRadius();

dde.draw(bxCylinder);
```

## Complete Example: Rendering PE World

Here's a complete pattern for rendering all rigid bodies in a PE world:

```cpp
#include <bgfx/bgfx.h>
#include <debugdraw/debugdraw.h>
#include <pe/core.h>

// Assuming you have a PE World
pe::WorldID world = /* your PE world */;

void renderPEWorld(uint8_t viewId)
{
    DebugDrawEncoder dde;
    dde.begin(viewId);

    // Set default rendering state
    dde.setWireframe(false);  // Solid rendering
    dde.setColor(0xffffffff);  // White

    // Iterate over all rigid bodies
    for (auto bodyIt = world->begin(); bodyIt != world->end(); ++bodyIt)
    {
        pe::ConstBodyID body = *bodyIt;

        // You can color-code bodies by type, state, etc.
        uint32_t color = body->isAwake() ? 0xff00ff00 : 0xff808080;  // Green if awake, gray if sleeping
        dde.setColor(color);

        // Check body geometry type and render accordingly
        if (body->hasType<pe::Sphere>())
        {
            pe::ConstSphereID sphere = static_body_cast<const pe::Sphere>(body);

            bx::Sphere bxSphere;
            const pe::Vec3& pos = sphere->getPosition();
            bxSphere.center = { pos[0], pos[1], pos[2] };
            bxSphere.radius = sphere->getRadius();

            dde.draw(bxSphere);
        }
        else if (body->hasType<pe::Box>())
        {
            pe::ConstBoxID box = static_body_cast<const pe::Box>(body);

            // Use OBB rendering
            bx::Obb bxObb;
            const pe::Rot3& rot = box->getRotation();
            const pe::Vec3& pos = box->getPosition();
            const pe::Vec3& lengths = box->getLengths();

            float sx = lengths[0] * 0.5f;
            float sy = lengths[1] * 0.5f;
            float sz = lengths[2] * 0.5f;

            bxObb.mtx[ 0] = rot[0][0] * sx;  bxObb.mtx[ 4] = rot[1][0] * sy;  bxObb.mtx[ 8] = rot[2][0] * sz;  bxObb.mtx[12] = pos[0];
            bxObb.mtx[ 1] = rot[0][1] * sx;  bxObb.mtx[ 5] = rot[1][1] * sy;  bxObb.mtx[ 9] = rot[2][1] * sz;  bxObb.mtx[13] = pos[1];
            bxObb.mtx[ 2] = rot[0][2] * sx;  bxObb.mtx[ 6] = rot[1][2] * sy;  bxObb.mtx[10] = rot[2][2] * sz;  bxObb.mtx[14] = pos[2];
            bxObb.mtx[ 3] = 0.0f;            bxObb.mtx[ 7] = 0.0f;            bxObb.mtx[11] = 0.0f;            bxObb.mtx[15] = 1.0f;

            dde.draw(bxObb);
        }
        else if (body->hasType<pe::Capsule>())
        {
            pe::ConstCapsuleID capsule = static_body_cast<const pe::Capsule>(body);

            const pe::Rot3& rot = capsule->getRotation();
            pe::Vec3 axis = rot[1];
            float halfLength = capsule->getLength() * 0.5f;
            const pe::Vec3& pos = capsule->getPosition();
            pe::Vec3 start = pos - axis * halfLength;
            pe::Vec3 end   = pos + axis * halfLength;

            bx::Capsule bxCapsule;
            bxCapsule.pos = { start[0], start[1], start[2] };
            bxCapsule.end = { end[0],   end[1],   end[2]   };
            bxCapsule.radius = capsule->getRadius();

            dde.draw(bxCapsule);
        }
        // ... Add other geometry types as needed
    }

    dde.end();
}
```

## Debug Draw Features

### Color Control

```cpp
// ABGR format (Alpha, Blue, Green, Red)
dde.setColor(0xffffffff);  // White (opaque)
dde.setColor(0xff0000ff);  // Red
dde.setColor(0xff00ff00);  // Green
dde.setColor(0xffff0000);  // Blue
dde.setColor(0x80ffffff);  // White, 50% transparent
```

### Wireframe vs Solid

```cpp
dde.setWireframe(true);   // Wireframe rendering
dde.setWireframe(false);  // Solid rendering
```

### Level of Detail

```cpp
dde.setLod(0);  // Lowest detail (fast, blocky)
dde.setLod(1);  // Low detail
dde.setLod(2);  // Medium detail
dde.setLod(3);  // High detail (default)
dde.setLod(UINT8_MAX);  // Maximum detail (slow)
```

**Note:** LOD affects tessellation of spheres, cylinders, capsules.

### Push/Pop State

```cpp
dde.push();  // Save current state (color, wireframe, transform, etc.)

dde.setColor(0xff0000ff);  // Temporary red color
// ... draw something red ...

dde.pop();  // Restore previous state
```

### Transform Hierarchy

```cpp
dde.push();
    float mtx[16];
    // ... build transform matrix ...
    dde.setTransform(mtx);  // Apply transform

    // All draws use this transform until pop()
    dde.draw(shape1);
    dde.draw(shape2);
dde.pop();  // Restore identity transform
```

## Additional Debug Visualization

### Draw Axes (for debugging orientation)

```cpp
dde.drawAxis(x, y, z, length = 1.0f, aspect = 0.7f, thickness = 0.0f);

// Example: Show body orientation
const pe::Vec3& pos = body->getPosition();
dde.drawAxis(pos[0], pos[1], pos[2], 1.0f);  // X=red, Y=green, Z=blue axes
```

### Draw Contact Points

```cpp
// For each contact in PE collision detection
for (auto& contact : contacts)
{
    dde.push();
        // Draw contact point as small sphere
        bx::Sphere contactPoint;
        contactPoint.center = { contact.pos[0], contact.pos[1], contact.pos[2] };
        contactPoint.radius = 0.05f;
        dde.setColor(0xffffff00);  // Yellow
        dde.draw(contactPoint);

        // Draw contact normal
        pe::Vec3 normalEnd = contact.pos + contact.normal * 0.5f;
        dde.setColor(0xffff0000);  // Blue
        dde.moveTo(contact.pos[0], contact.pos[1], contact.pos[2]);
        dde.lineTo(normalEnd[0], normalEnd[1], normalEnd[2]);
    dde.pop();
}
```

### Draw Grid (ground plane)

```cpp
// Draw XZ ground plane at Y=0
bx::Vec3 normal = { 0.0f, 1.0f, 0.0f };  // Y-up
bx::Vec3 center = { 0.0f, 0.0f, 0.0f };
dde.setColor(0xff404040);  // Dark gray
dde.drawGrid(normal, center, 50, 1.0f);  // 50x50 grid, 1-meter spacing
```

## Integration into main.cpp

Here's how to integrate debug draw into your existing `main.cpp`:

```cpp
#include <debugdraw/debugdraw.h>
#include <pe/core.h>

// In init()
ddInit();

// Create PE world
pe::WorldID world = pe::theWorld();
// ... set up simulation (gravity, bodies, etc.) ...

// In update() - before bgfx::frame()
{
    // Step physics
    world->simulationStep(dt);

    // Render debug view of physics world
    DebugDrawEncoder dde;
    dde.begin(0);

    // Draw ground plane
    bx::Vec3 up = { 0.0f, 1.0f, 0.0f };
    bx::Vec3 origin = { 0.0f, 0.0f, 0.0f };
    dde.setColor(0xff303030);
    dde.drawGrid(up, origin, 20, 1.0f);

    // Draw all rigid bodies
    for (auto bodyIt = world->begin(); bodyIt != world->end(); ++bodyIt)
    {
        // ... render each body as shown above ...
    }

    dde.end();
}

// In shutdown()
ddShutdown();
```

## Performance Considerations

**Debug Draw is for prototyping, not production:**
- ✅ Perfect for learning and quick iteration
- ✅ Great for debugging physics behavior
- ❌ Not optimized for thousands of objects
- ❌ Generates geometry every frame (no caching)

**When to switch to optimized rendering:**
- You have > 100 dynamic bodies
- You need 60 FPS with complex scenes
- You want custom shaders and materials

**Hybrid approach:**
- Use debug draw for visualization/debugging overlays
- Use optimized rendering (vertex buffers) for main scene
- Example: Render bodies with meshes, use debug draw for contact points and normals

## Minimal Working Example

```cpp
#include <bgfx/bgfx.h>
#include <debugdraw/debugdraw.h>
#include <pe/core.h>

int main()
{
    // ... SDL/bgfx initialization ...

    ddInit();

    // Create simple PE world
    pe::WorldID world = pe::theWorld();
    world->setGravity(0, -9.81, 0);

    // Create ground plane
    pe::PlaneID ground = pe::createPlane(0, pe::Vec3(0, 1, 0), 0, pe::iron);

    // Create falling sphere
    pe::SphereID sphere = pe::createSphere(1, pe::Vec3(0, 5, 0), 0.5, pe::iron);
    sphere->setLinearVel(0, 0, 0);

    // Main loop
    while (!quit)
    {
        // Step physics
        world->simulationStep(0.016f);  // 60 Hz

        // Render
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff);
        bgfx::setViewRect(0, 0, 0, width, height);
        bgfx::setViewTransform(0, view, proj);

        DebugDrawEncoder dde;
        dde.begin(0);

        // Draw ground
        bx::Vec3 up = { 0.0f, 1.0f, 0.0f };
        bx::Vec3 origin = { 0.0f, 0.0f, 0.0f };
        dde.drawGrid(up, origin, 10, 1.0f);

        // Draw sphere
        const pe::Vec3& pos = sphere->getPosition();
        bx::Sphere bxSphere;
        bxSphere.center = { pos[0], pos[1], pos[2] };
        bxSphere.radius = sphere->getRadius();
        dde.setColor(0xff00ff00);  // Green
        dde.draw(bxSphere);

        dde.end();

        bgfx::frame();
    }

    ddShutdown();
    // ... cleanup ...
}
```

## Next Steps

1. **Study example-29-debugdraw** - Run it and see all the debug primitives
2. **Add debug draw to main.cpp** - Just the initialization
3. **Create simple PE scene** - One sphere, one plane
4. **Render with debug draw** - Get the sphere bouncing visually
5. **Expand** - Add more bodies, different shapes
6. **Add debug overlays** - Contact points, velocity vectors
7. **Later** - Replace with optimized rendering when needed

For reference on optimized rendering, see:
- `doc/bgfx-01-cubes-study.md` - Vertex buffer-based rendering
- `doc/pe-bgfx-integration-strategy.md` - Full integration plan
