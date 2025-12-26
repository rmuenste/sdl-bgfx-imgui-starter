# BGFX Example Framework Architecture

This document explains how the bgfx example framework works, including the entry system, macro expansion, and the multi-app demo switcher.

## Table of Contents
- [Basic Example Structure](#basic-example-structure)
- [The ENTRY_IMPLEMENT_MAIN Macro](#the-entry_implement_main-macro)
- [Two Build Modes](#two-build-modes)
- [The Multi-App List System](#the-multi-app-list-system)
- [Window Size Configuration](#window-size-configuration)
- [Runtime App Switching](#runtime-app-switching)

## Basic Example Structure

Every bgfx example follows this pattern:

```cpp
class ExampleHelloWorld : public entry::AppI
{
public:
    ExampleHelloWorld(const char* _name, const char* _description, const char* _url)
        : entry::AppI(_name, _description, _url)
    {}

    void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
    {
        // Initialize bgfx, set up rendering state
    }

    int shutdown() override
    {
        // Clean up resources
        bgfx::shutdown();
        return 0;
    }

    bool update() override
    {
        // Called every frame
        // Return false to exit
        return true;
    }
};

ENTRY_IMPLEMENT_MAIN(
    ExampleHelloWorld,
    "00-helloworld",
    "Initialization and debug text.",
    "https://bkaradzic.github.io/bgfx/examples.html#helloworld"
);
```

**Key locations:**
- Example code: `bgfx/examples/00-helloworld/helloworld.cpp`
- Framework header: `bgfx/examples/common/entry/entry.h`
- Framework implementation: `bgfx/examples/common/entry/entry.cpp`

## The ENTRY_IMPLEMENT_MAIN Macro

The `ENTRY_IMPLEMENT_MAIN` macro has two different expansions depending on build configuration.

**Location:** `bgfx/examples/common/entry/entry.h:22-36`

### Mode 1: Individual Executables (ENTRY_CONFIG_IMPLEMENT_MAIN=1)

When building separate executables for each example:

```cpp
#define ENTRY_IMPLEMENT_MAIN(_app, ...)                 \
    int _main_(int _argc, char** _argv)                 \
    {                                                   \
        _app app(__VA_ARGS__);                          \
        return entry::runApp(&app, _argc, _argv);       \
    }
```

This expands to create a `_main_()` function that:
1. Instantiates your app class with the provided arguments
2. Calls `entry::runApp()` to run it

**Example expansion:**
```cpp
int _main_(int _argc, char** _argv)
{
    ExampleHelloWorld app("00-helloworld", "Initialization and debug text.", "https://...");
    return entry::runApp(&app, _argc, _argv);
}
```

### Mode 2: Combined Executable (ENTRY_CONFIG_IMPLEMENT_MAIN=0, default)

When building all examples into one binary:

```cpp
#define ENTRY_IMPLEMENT_MAIN(_app, ...) \
    _app s_ ## _app ## App(__VA_ARGS__)
```

This creates a **global static instance** that auto-registers itself.

**Example expansion:**
```cpp
ExampleHelloWorld s_ExampleHelloWorldApp("00-helloworld", "Initialization and debug text.", "https://...");
```

**Auto-registration mechanism** (`entry.cpp:456-470`):
- The `AppI` constructor adds the instance to a global linked list `s_apps`
- All examples linked into the binary automatically register themselves before `main()` runs
- The framework's `main()` function iterates the list to find and run the selected app

## Two Build Modes

The build system (`bgfx/scripts/genie.lua:450-500`) supports two modes:

### Combined Mode (`_combined = true`)

```lua
project ("examples")
    kind "WindowedApp"

for _, name in ipairs({...}) do
    files {
        path.join(BGFX_DIR, "examples", name, "**.cpp"),
    }
end
```

**Result:**
- One executable called `examples`
- Contains all ~50 example .cpp files
- Does NOT define `ENTRY_CONFIG_IMPLEMENT_MAIN=1`
- All examples auto-register via global instances
- Creates the multi-app demo switcher

### Individual Mode (`_combined = false`)

```lua
for _, name in ipairs({...}) do
    project ("example-" .. name)
        kind "WindowedApp"

    files {
        path.join(BGFX_DIR, "examples", name, "**.cpp"),
    }

    defines {
        "ENTRY_CONFIG_IMPLEMENT_MAIN=1",
    }
end
```

**Result:**
- Separate executables: `example-00-helloworld`, `example-01-cubes`, etc.
- Each contains only its own .cpp file
- Each defines its own `_main_()` function
- Standard standalone executables

## The Multi-App List System

When built in combined mode, all examples register themselves in a linked list.

**Data structure** (`entry.cpp:374-376`):
```cpp
static AppI*    s_currentApp = NULL;
static AppI*    s_apps       = NULL;  // Head of linked list
static uint32_t s_numApps    = 0;
```

**Registration** (`entry.cpp:456-470`):
```cpp
AppI::AppI(const char* _name, const char* _description, const char* _url)
{
    AppInternal* ai = (AppInternal*)m_internal;
    ai->m_name        = _name;
    ai->m_description = _description;
    ai->m_url         = _url;
    ai->m_next        = s_apps;  // Point to current head

    s_apps = this;  // Become new head
    s_numApps++;
}
```

This creates a linked list of all registered examples.

### App Selection at Launch

The `main()` function (`entry.cpp:620-643`) selects which app to run:

```cpp
const char* find = "";
if (1 < _argc)
{
    find = _argv[_argc-1];  // Last command-line argument
}

AppI* selected = NULL;
for (AppI* app = getFirstApp(); NULL != app; app = app->getNext() )
{
    if (NULL == selected && !bx::strFindI(app->getName(), find).isEmpty() )
    {
        selected = app;
    }
}

result = runApp(getCurrentApp(selected), _argc, _argv);
```

**Usage examples:**
```bash
./examples                    # Runs first app alphabetically
./examples helloworld         # Runs 00-helloworld
./examples cubes              # Runs 01-cubes
```

## Window Size Configuration

The default window size is defined in `bgfx/examples/common/entry/entry_p.h:49-54`:

```cpp
#if !defined(ENTRY_DEFAULT_WIDTH) && !defined(ENTRY_DEFAULT_HEIGHT)
#   define ENTRY_DEFAULT_WIDTH  1280
#   define ENTRY_DEFAULT_HEIGHT 720
#elif !defined(ENTRY_DEFAULT_WIDTH) || !defined(ENTRY_DEFAULT_HEIGHT)
#   error "Both ENTRY_DEFAULT_WIDTH and ENTRY_DEFAULT_HEIGHT must be defined."
#endif
```

**Default:** 1280x720

**Flow:**
1. `entry.cpp:27-28` - Initializes static variables with defaults
2. `entry.cpp:616` - `main()` sets window size via `setWindowSize(kDefaultWindowHandle, ENTRY_DEFAULT_WIDTH, ENTRY_DEFAULT_HEIGHT)`
3. `entry.cpp:533` - `runApp()` calls `_app->init(_argc, _argv, s_width, s_height)`
4. Your example's `init()` receives the width and height parameters

**To customize:** Define `ENTRY_DEFAULT_WIDTH` and `ENTRY_DEFAULT_HEIGHT` before including entry headers, or pass them as compile-time defines.

## Runtime App Switching

This is the key feature of combined mode: switching between examples without closing the application.

### The Restart Mechanism

**Command handler** (`entry.cpp:405-444`):
```cpp
int cmdApp(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
{
    if (0 == bx::strCmp(_argv[1], "restart") )
    {
        if (0 == bx::strCmp(_argv[2], "next") )
        {
            AppI* next = getNextWrap(getCurrentApp() );
            bx::strCopy(s_restartArgs, BX_COUNTOF(s_restartArgs), next->getName() );
            return bx::kExitSuccess;
        }
        else if (0 == bx::strCmp(_argv[2], "prev") )
        {
            // Find previous app in list
        }
        // ... or restart specific app by name
    }
}
```

**The restart loop** (`entry.cpp:626-660`):
```cpp
restart:
    AppI* selected = NULL;

    // Find app matching s_restartArgs
    for (AppI* app = getFirstApp(); NULL != app; app = app->getNext() )
    {
        if (NULL == selected && !bx::strFindI(app->getName(), find).isEmpty() )
        {
            selected = app;
        }
    }

    result = runApp(getCurrentApp(selected), _argc, _argv);

    if (0 != bx::strLen(s_restartArgs) )
    {
        find = s_restartArgs;
        goto restart;  // Switch to different app!
    }
```

### How It Works

1. User types console command (e.g., `app restart next`) or presses a hotkey
2. Command handler sets `s_restartArgs` to the name of the target example
3. Current app's `update()` loop detects the command and returns `false`
4. `runApp()` returns, exiting the current app
5. The `if (0 != bx::strLen(s_restartArgs))` check triggers
6. `goto restart;` jumps back to app selection
7. The new app is selected and `runApp()` is called with it
8. Window stays open, rendering context persists, only the `AppI` instance changes

### Console Commands

Available commands when running the combined examples binary:

- `app restart` - Restart current example
- `app restart next` - Switch to next example in list
- `app restart prev` - Switch to previous example in list
- `app restart <name>` - Switch to specific example by name

### Benefits of Combined Mode

1. **Quick exploration** - Browse all examples without relaunching
2. **Demos at events** - Smooth transitions between examples
3. **Development** - Test changes across multiple examples rapidly
4. **Distribution** - Ship one binary instead of 50 separate executables
5. **Learning** - Compare different techniques side-by-side

## Summary

The bgfx example framework is a sophisticated system that:

- Uses C++ macro magic to support two build modes from the same source
- In combined mode, leverages global static initialization for auto-registration
- Implements a runtime demo switcher via a simple goto-based restart loop
- Provides a clean, consistent interface for all examples via the `AppI` base class
- Reduces boilerplate while maintaining flexibility

This design is particularly elegant because it achieves two very different build configurations (standalone vs. combined) with minimal code changes in the examples themselves - just a different macro expansion controlled by one `#define`.
