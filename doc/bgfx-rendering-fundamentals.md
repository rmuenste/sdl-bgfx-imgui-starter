# BGFX Rendering Fundamentals

This document explains the core concepts and patterns used in bgfx applications, based on analysis of the 00-helloworld example and the entry framework.

## Table of Contents
- [The Application Lifecycle](#the-application-lifecycle)
- [Understanding update() - The Main Loop](#understanding-update---the-main-loop)
- [The Three Core Methods](#the-three-core-methods)
- [The bgfx::frame() Concept](#the-bgfxframe-concept)
- [Views in bgfx](#views-in-bgfx)
- [The Rendering Pipeline](#the-rendering-pipeline)
- [What HelloWorld Actually Demonstrates](#what-helloworld-actually-demonstrates)

## The Application Lifecycle

Every bgfx application follows this lifecycle pattern, managed by the entry framework:

```cpp
int runApp(AppI* _app, int _argc, const char* const* _argv)
{
    setWindowSize(kDefaultWindowHandle, s_width, s_height);

    _app->init(_argc, _argv, s_width, s_height);  // 1. ONE-TIME INITIALIZATION
    bgfx::frame();

    while (_app->update() )  // 2. MAIN RENDERING LOOP
    {
        if (0 != bx::strLen(s_restartArgs) )
        {
            break;  // Support for switching examples
        }
    }

    return _app->shutdown();  // 3. ONE-TIME CLEANUP
}
```

**Location:** `bgfx/examples/common/entry/entry.cpp:531-552`

### The Three Phases

1. **Initialization** - `init()` called once at startup
2. **Main Loop** - `update()` called every frame while it returns `true`
3. **Shutdown** - `shutdown()` called once before exit

## Understanding update() - The Main Loop

### It IS the Main Rendering Loop

`update()` is **not** a specialized callback - it **IS** the main rendering loop body. Each call represents one frame.

**Traditional game loop pattern:**
```cpp
init();

while (!shouldQuit)  // ← You control the loop
{
    processInput();
    updateGameState();
    render();
}

cleanup();
```

**bgfx pattern:**
```cpp
init()  // ← Called once by framework

// Framework does: while (update())
bool update()  // ← This IS the loop body, called every frame
{
    processInput();
    updateGameState();
    render();
    bgfx::frame();  // Tell bgfx to render the frame

    return !shouldQuit;  // true = continue, false = exit
}

shutdown()  // ← Called once by framework
```

### Why the Framework Controls the Loop

The entry framework controls the loop (instead of you writing `while(true)`) for several reasons:

**1. Platform Abstraction**

Different platforms need different loop structures:

```cpp
#if BX_PLATFORM_EMSCRIPTEN
    // Web browsers control the loop via requestAnimationFrame
    emscripten_set_main_loop(&updateApp, -1, 1);
#else
    // Traditional platforms use a while loop
    while (_app->update()) { }
#endif
```

**2. Example Switching**

The framework can break out of the loop to switch to another example:

```cpp
while (_app->update() )
{
    if (0 != bx::strLen(s_restartArgs) )
    {
        break;  // Exit current app, start another
    }
}
```

**3. Centralized Event Handling**

The framework handles window events (resize, close, etc.) before your code runs.

### The Update Return Value

```cpp
bool update() override
{
    if (!entry::processEvents(...))
    {
        // Normal frame processing
        // ...
        return true;   // ← Continue running (next frame please!)
    }

    return false;  // ← User wants to exit (stop the loop)
}
```

- **`return true`** - "Keep calling me, I want to render another frame"
- **`return false`** - "I'm done, exit the application"

## The Three Core Methods

Every bgfx application must implement these three methods from `entry::AppI`:

### 1. init() - One-Time Setup

**Purpose:** Initialize bgfx and create resources

**Location in helloworld:** `helloworld.cpp:23-54`

```cpp
void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
{
    // Parse command-line arguments
    Args args(_argc, _argv);

    // Store window dimensions
    m_width  = _width;
    m_height = _height;
    m_debug  = BGFX_DEBUG_TEXT;
    m_reset  = BGFX_RESET_VSYNC;

    // Initialize bgfx
    bgfx::Init init;
    init.type     = args.m_type;              // Graphics API (D3D, OpenGL, Vulkan, Metal)
    init.vendorId = args.m_pciId;             // GPU selection
    init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
    init.platformData.ndt  = entry::getNativeDisplayHandle();
    init.resolution.width  = m_width;
    init.resolution.height = m_height;
    init.resolution.reset  = m_reset;
    bgfx::init(init);

    // Enable debug features
    bgfx::setDebug(m_debug);

    // Configure view 0 (the default rendering pass)
    bgfx::setViewClear(0
        , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
        , 0x303030ff    // Clear color (dark gray, RGBA)
        , 1.0f          // Depth clear value
        , 0             // Stencil clear value
    );

    // Initialize ImGui
    imguiCreate();
}
```

**Typical tasks in init():**
- Initialize bgfx with platform and API settings
- Create vertex/index buffers
- Load and compile shaders
- Load textures and models
- Set up initial view configurations
- Create framebuffers and render targets
- Initialize UI systems

### 2. update() - Called Every Frame

**Purpose:** The main rendering loop body

**Location in helloworld:** `helloworld.cpp:66-122`

```cpp
bool update() override
{
    // Process window/input events
    // Returns true if user wants to exit
    if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
    {
        // ═══════════════════════════════════════
        // FRAME BEGINS - Everything here runs once per frame
        // ═══════════════════════════════════════

        // 1. Process input for ImGui
        imguiBeginFrame(m_mouseState.m_mx
            ,  m_mouseState.m_my
            , (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
            | (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
            | (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
            ,  m_mouseState.m_mz
            , uint16_t(m_width)
            , uint16_t(m_height)
        );

        showExampleDialog(this);
        imguiEndFrame();

        // 2. Set up the viewport for view 0
        bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));

        // 3. Ensure view 0 gets cleared even if nothing is drawn
        bgfx::touch(0);

        // 4. Submit rendering commands (debug text in this case)
        bgfx::dbgTextClear();
        bgfx::dbgTextImage(
              bx::max<uint16_t>(uint16_t(m_width /2/8 ), 20)-20
            , bx::max<uint16_t>(uint16_t(m_height/2/16),  6)-6
            , 40
            , 12
            , s_logo
            , 160
        );
        bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI escape codes...");

        const bgfx::Stats* stats = bgfx::getStats();
        bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters."
            , stats->width
            , stats->height
            , stats->textWidth
            , stats->textHeight
        );

        // 5. END OF FRAME - Tell bgfx to render everything
        bgfx::frame();

        // ═══════════════════════════════════════
        // FRAME ENDS
        // ═══════════════════════════════════════

        return true;  // Continue to next frame
    }

    return false;  // User requested exit
}
```

**Typical tasks in update():**
- Process input events
- Update game/simulation state
- Update animations, physics, AI
- Set view transforms (camera matrices)
- Submit geometry for rendering
- Call `bgfx::frame()` to execute rendering
- Return `true` to continue or `false` to exit

### 3. shutdown() - One-Time Cleanup

**Purpose:** Clean up resources and shut down bgfx

**Location in helloworld:** `helloworld.cpp:56-64`

```cpp
virtual int shutdown() override
{
    // Destroy ImGui
    imguiDestroy();

    // Shutdown bgfx (automatically destroys all bgfx resources)
    bgfx::shutdown();

    return 0;
}
```

**Typical tasks in shutdown():**
- Destroy UI systems
- Call `bgfx::shutdown()` (automatically destroys all bgfx objects)
- Clean up any non-bgfx resources
- Save settings/state if needed

**Important:** You don't need to manually destroy individual bgfx resources (buffers, textures, shaders) - `bgfx::shutdown()` handles cleanup automatically.

## The bgfx::frame() Concept

`bgfx::frame()` is the **most critical function** in the rendering loop.

### Command Submission vs Execution

bgfx uses a **deferred rendering model**:

```cpp
// Everything before bgfx::frame() is COMMAND SUBMISSION
// These don't render immediately - they just queue commands

bgfx::setState(BGFX_STATE_DEFAULT);
bgfx::setVertexBuffer(0, vbh);
bgfx::setIndexBuffer(ibh);
bgfx::submit(0, program);

bgfx::dbgTextPrintf(0, 0, 0x0f, "Hello");

// ← At this point, NOTHING has been rendered yet!

bgfx::frame();  // ← NOW everything gets rendered
```

### What bgfx::frame() Does

When you call `bgfx::frame()`, bgfx:

1. **Sorts** all submitted draw calls for optimal GPU performance
2. **Translates** commands to the native graphics API (D3D/OpenGL/Vulkan/Metal)
3. **Executes** all rendering commands on the GPU
4. **Presents** the rendered frame to the screen
5. **Prepares** for the next frame

### The Frame Barrier

```cpp
// Frame N begins
bgfx::setViewRect(0, 0, 0, 1280, 720);
bgfx::submit(0, program);
bgfx::frame();  // ← Frame N ends and is rendered

// Frame N+1 begins
bgfx::setViewRect(0, 0, 0, 1280, 720);
bgfx::submit(0, program);
bgfx::frame();  // ← Frame N+1 ends and is rendered
```

Each call to `bgfx::frame()` represents one frame. Typically called once per `update()`.

### Threading Model

bgfx runs rendering on a separate thread:

```
Main Thread                  Render Thread
-----------                  -------------
update() {
  submit commands ------>   [queued]
  bgfx::frame()  ------>    Execute commands
  return                    Render to GPU
}                           Present frame
update() {
  submit commands ------>   [queued]
  ...
```

This allows the CPU to prepare the next frame while the GPU renders the current frame.

## Views in bgfx

bgfx organizes rendering into **views**, which are like rendering passes or layers.

### What is a View?

A view is a rendering pass with its own:
- Viewport (screen region)
- Clear state (color, depth, stencil)
- Transform (view/projection matrices)
- Framebuffer (render target)
- Sort order

### View 0 - The Default View

Every bgfx application has at least view 0:

```cpp
// Configure view 0 to clear to dark gray
bgfx::setViewClear(0
    , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
    , 0x303030ff    // RGBA color
    , 1.0f          // Depth
    , 0             // Stencil
);

// Set view 0's viewport to full screen
bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));

// Submit a draw call to view 0
bgfx::submit(0, program);
```

### The Touch Command

```cpp
bgfx::touch(0);
```

**Purpose:** Ensures view 0 gets processed (and cleared) even if no geometry is submitted to it.

Without `touch()`, if you don't submit any draw calls to a view, it won't be cleared either. `touch()` forces the view to execute its clear operations.

### Multiple Views

More complex applications use multiple views for different rendering passes:

```cpp
// View 0: Render scene to shadow map
bgfx::setViewFrameBuffer(0, shadowMapFB);
bgfx::setViewClear(0, BGFX_CLEAR_DEPTH, 0x0, 1.0f, 0);
// ... submit shadow casters ...

// View 1: Render main scene
bgfx::setViewFrameBuffer(1, BGFX_INVALID_HANDLE);  // Use backbuffer
bgfx::setViewClear(1, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
// ... submit scene geometry ...

// View 2: Render UI on top
// ... submit UI elements ...

bgfx::frame();  // All views rendered in order
```

Views execute in numerical order (0, 1, 2, ...).

## The Rendering Pipeline

Here's what happens during a typical frame:

```cpp
bool update() override
{
    // ═══════════════════════════════════════
    // 1. EVENT PROCESSING
    // ═══════════════════════════════════════
    if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
    {
        // ═══════════════════════════════════════
        // 2. INPUT HANDLING
        // ═══════════════════════════════════════
        // Process mouse, keyboard, gamepad input
        // Update camera based on input

        // ═══════════════════════════════════════
        // 3. STATE UPDATE
        // ═══════════════════════════════════════
        // Update game logic, physics, animations
        // Calculate transform matrices

        // ═══════════════════════════════════════
        // 4. VIEW SETUP
        // ═══════════════════════════════════════
        bgfx::setViewRect(0, 0, 0, m_width, m_height);
        bgfx::setViewTransform(0, view, proj);

        // ═══════════════════════════════════════
        // 5. COMMAND SUBMISSION
        // ═══════════════════════════════════════
        // For each object to render:
        bgfx::setTransform(model);              // Model matrix
        bgfx::setVertexBuffer(0, vbh);          // Vertex data
        bgfx::setIndexBuffer(ibh);              // Index data
        bgfx::setState(BGFX_STATE_DEFAULT);     // Render state
        bgfx::setTexture(0, s_texture, texture); // Textures
        bgfx::submit(0, program);               // Draw call

        // ═══════════════════════════════════════
        // 6. FRAME EXECUTION
        // ═══════════════════════════════════════
        bgfx::frame();  // Render everything and present

        return true;  // Continue
    }

    return false;  // Exit
}
```

## What HelloWorld Actually Demonstrates

The 00-helloworld example is intentionally minimal. It demonstrates:

### What It DOES Show

1. **Initialization Pattern**
   - How to initialize bgfx with platform integration
   - Setting up the `bgfx::Init` structure
   - Connecting to the window system

2. **View Configuration**
   - Setting up view 0 with a clear color
   - Using `bgfx::setViewClear()` and `bgfx::setViewRect()`

3. **The Rendering Loop**
   - The `update()` → `bgfx::frame()` → `return true` pattern
   - Event processing with `entry::processEvents()`

4. **Debug Text Rendering**
   - Using `bgfx::dbgTextPrintf()` for simple output
   - The built-in character-mode text system
   - Getting rendering statistics with `bgfx::getStats()`

5. **Frame Submission**
   - The critical `bgfx::frame()` call
   - Using `bgfx::touch()` to ensure clearing

6. **Shutdown Pattern**
   - Proper cleanup with `bgfx::shutdown()`

### What It Does NOT Show

- **3D Geometry** - No vertices, indices, or meshes
- **Shaders** - No vertex/fragment shader programs
- **Textures** - No texture loading or sampling
- **Transforms** - No matrices or camera setup
- **Render States** - No depth testing, blending, etc.

### The Philosophy

HelloWorld answers the question: **"Can you initialize bgfx and get a window with debug text?"**

It's a sanity check that proves:
- bgfx compiles and links correctly
- Platform integration works (window handles)
- The graphics API (D3D/OpenGL/Vulkan/Metal) initializes
- The rendering loop runs at the correct framerate
- Debug output functions

It's **"step 0"** - proving the library works before learning graphics concepts.

### For Actual Rendering

To see real 3D rendering, examine:
- **01-cubes** - First example with geometry, shaders, and transforms
- **03-raymarch** - Shader-based rendering techniques
- **04-mesh** - Loading and rendering mesh files
- **06-bump** - Normal mapping and texturing

## Summary

The bgfx rendering fundamentals are:

1. **Three-phase lifecycle**: `init()` → `update()` (loop) → `shutdown()`
2. **`update()` IS the main loop**: Called every frame, returns `true` to continue
3. **Command submission model**: Set up state, submit commands, then `bgfx::frame()` renders
4. **Views organize rendering**: Different passes, viewports, and render targets
5. **Platform abstraction**: Framework handles loop differences across platforms
6. **Threading**: Render thread executes while main thread prepares next frame

This design provides:
- Clean separation of initialization, rendering, and cleanup
- Platform portability (desktop, mobile, web)
- Efficient multi-threaded rendering
- Flexible multi-pass rendering via views
- Simple API for complex rendering pipelines
