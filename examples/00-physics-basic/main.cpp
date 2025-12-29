#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <debugdraw/debugdraw.h>

#include <SDL.h>
#include <SDL_syswm.h>

// PE requires config.h to be included first for PE_PUBLIC and other macros
#include <config.h>
#include <pe/core.h>

#include "bgfx-imgui/imgui_impl_bgfx.h"
#include "imgui.h"
#include "sdl-imgui/imgui_impl_sdl2.h"

#if BX_PLATFORM_EMSCRIPTEN
#include "emscripten.h"
#endif

// Global PE objects
pe::WorldID g_world;
pe::PlaneID g_ground;

// Color palette for spheres
static const uint32_t SPHERE_COLORS[] = {
    0xff00ff00,  // Green
    0xff0000ff,  // Red
    0xffff0000,  // Blue
    0xff00ffff,  // Yellow
    0xffff00ff,  // Magenta
    0xffffff00,  // Cyan
    0xffffffff,  // White
    0xffff8000,  // Orange
};
const int NUM_COLORS = sizeof(SPHERE_COLORS) / sizeof(SPHERE_COLORS[0]);

struct context_t
{
    SDL_Window* window = nullptr;

    float cam_pitch = 0.0f;
    float cam_yaw = 0.0f;
    float rot_scale = 0.01f;

    int prev_mouse_x = 0;
    int prev_mouse_y = 0;

    int width = 0;
    int height = 0;

    bool quit = false;
};

void main_loop(void* data)
{
    auto context = static_cast<context_t*>(data);

    // Step physics
    pe::real dt = 1.0 / 60.0;
    g_world->simulationStep(dt);

    // Event handling
    for (SDL_Event current_event; SDL_PollEvent(&current_event) != 0;) {
        ImGui_ImplSDL2_ProcessEvent(&current_event);
        if (current_event.type == SDL_QUIT) {
            context->quit = true;
            break;
        }
    }

    // ImGui frame
    ImGui_Implbgfx_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Simple info window
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin("Physics Info");
    ImGui::Text("Example: 00-physics-basic");
    ImGui::Text("Bodies: %lu", g_world->size());
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

    // Camera controls
    if (!ImGui::GetIO().WantCaptureMouse) {
        int mouse_x, mouse_y;
        const int buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        if ((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0) {
            int delta_x = mouse_x - context->prev_mouse_x;
            int delta_y = mouse_y - context->prev_mouse_y;
            context->cam_yaw += float(-delta_x) * context->rot_scale;
            context->cam_pitch += float(-delta_y) * context->rot_scale;
        }
        context->prev_mouse_x = mouse_x;
        context->prev_mouse_y = mouse_y;
    }

    // Camera matrices
    float cam_rotation[16];
    bx::mtxRotateXYZ(cam_rotation, context->cam_pitch, context->cam_yaw, 0.0f);

    float cam_translation[16];
    bx::mtxTranslate(cam_translation, 0.0f, 2.0f, -8.0f);

    float cam_transform[16];
    bx::mtxMul(cam_transform, cam_translation, cam_rotation);

    float view[16];
    bx::mtxInverse(view, cam_transform);

    float proj[16];
    bx::mtxProj(proj, 60.0f, float(context->width) / float(context->height),
                0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

    bgfx::setViewTransform(0, view, proj);

    // Debug draw rendering
    DebugDrawEncoder dde;
    dde.begin(0);

    // Draw ground grid
    bx::Vec3 up = { 0.0f, 1.0f, 0.0f };
    bx::Vec3 origin = { 0.0f, 0.0f, 0.0f };
    dde.setColor(0xff404040);
    dde.drawGrid(up, origin, 20, 1.0f);

    // Render all spheres
    int sphereIndex = 0;
    for (auto bodyIt = g_world->begin(); bodyIt != g_world->end(); ++bodyIt)
    {
        pe::BodyID body = *bodyIt;

        if (body->getType() == pe::sphereType) {
            pe::SphereID sphere = pe::static_body_cast<pe::Sphere>(body);

            // Get sphere data
            const pe::Vec3& pos = sphere->getPosition();
            const pe::Rot3& rot = sphere->getRotation();
            float radius = static_cast<float>(sphere->getRadius());

            // Select color
            uint32_t color = SPHERE_COLORS[sphereIndex % NUM_COLORS];

            // Draw sphere
            bx::Sphere bxSphere;
            bxSphere.center = { static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]) };
            bxSphere.radius = radius;
            dde.setColor(color);
            dde.draw(bxSphere);

            // Draw orientation axes
            float axisLength = radius * 1.5f;

            // X-axis (red) - rot is column-major: [0,3,6] [1,4,7] [2,5,8]
            dde.setColor(0xff0000ff);
            dde.moveTo(static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]));
            dde.lineTo(
                static_cast<float>(pos[0] + rot[0] * axisLength),
                static_cast<float>(pos[1] + rot[3] * axisLength),
                static_cast<float>(pos[2] + rot[6] * axisLength)
            );

            // Y-axis (green)
            dde.setColor(0xff00ff00);
            dde.moveTo(static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]));
            dde.lineTo(
                static_cast<float>(pos[0] + rot[1] * axisLength),
                static_cast<float>(pos[1] + rot[4] * axisLength),
                static_cast<float>(pos[2] + rot[7] * axisLength)
            );

            // Z-axis (blue)
            dde.setColor(0xffff0000);
            dde.moveTo(static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]));
            dde.lineTo(
                static_cast<float>(pos[0] + rot[2] * axisLength),
                static_cast<float>(pos[1] + rot[5] * axisLength),
                static_cast<float>(pos[2] + rot[8] * axisLength)
            );

            sphereIndex++;
        }
    }

    dde.end();

    bgfx::frame();

#if BX_PLATFORM_EMSCRIPTEN
    if (context->quit) {
        emscripten_cancel_main_loop();
    }
#endif
}

int main(int argc, char** argv)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    const int width = 1024;
    const int height = 768;
    SDL_Window* window = SDL_CreateWindow(
        "00-physics-basic", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height, SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        printf("Window could not be created. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

#if !BX_PLATFORM_EMSCRIPTEN
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        printf("SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    bgfx::renderFrame();
#endif

    // Initialize bgfx
    bgfx::PlatformData pd{};
#if BX_PLATFORM_WINDOWS
    pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
    pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_LINUX
    pd.ndt = wmi.info.x11.display;
    pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif BX_PLATFORM_EMSCRIPTEN
    pd.nwh = (void*)"#canvas";
#endif

    bgfx::Init bgfx_init;
    bgfx_init.type = bgfx::RendererType::Count;
    bgfx_init.resolution.width = width;
    bgfx_init.resolution.height = height;
    bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx_init.platformData = pd;
    bgfx::init(bgfx_init);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width, height);

    // Initialize ImGui
    ImGui::CreateContext();
    ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
    ImGui_ImplSDL2_InitForD3D(window);
#elif BX_PLATFORM_OSX
    ImGui_ImplSDL2_InitForMetal(window);
#elif BX_PLATFORM_LINUX || BX_PLATFORM_EMSCRIPTEN
    ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
#endif

    // Initialize debug draw
    ddInit();

    // Initialize PE physics world
    g_world = pe::theWorld();
    g_world->setGravity(pe::Vec3(0.0, -9.81, 0.0));

    unsigned int idx = 0;

    // Create material
    pe::MaterialID material = pe::createMaterial("default", 1.0, 0.3, 0.5, 0.05, 0.2, 80, 100, 10, 11);

    // Create ground plane
    g_ground = pe::createPlane(idx++, 0.0, 1.0, 0.0, 0.0, material);

    // Create some spheres
    pe::createSphere(idx++, pe::Vec3(0.0, 5.0, 0.0), 0.5, material);
    pe::createSphere(idx++, pe::Vec3(1.5, 6.0, 0.0), 0.4, material);
    pe::createSphere(idx++, pe::Vec3(-1.5, 7.0, 0.0), 0.6, material);

    // Setup context
    context_t context;
    context.width = width;
    context.height = height;
    context.window = window;

    // Main loop
#if BX_PLATFORM_EMSCRIPTEN
    emscripten_set_main_loop_arg(main_loop, &context, -1, 1);
#else
    while (!context.quit) {
        main_loop(&context);
    }
#endif

    // Cleanup
    ddShutdown();

    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();
    ImGui::DestroyContext();

    bgfx::shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
