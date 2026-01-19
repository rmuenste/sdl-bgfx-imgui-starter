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
#include "sphere-renderer/sphere_renderer.h"

#include <cmath>
#include <vector>

#if BX_PLATFORM_EMSCRIPTEN
#include "emscripten.h"
#endif

// Global PE objects
pe::WorldID g_world;
pe::PlaneID g_ground;

// Global sphere renderer
SphereRenderer g_sphere_renderer;

// Initial state storage for reset functionality
struct BodyInitialState {
    pe::Vec3 position;
    pe::Vec3 velocity;
    pe::Vec3 angular_velocity;
    pe::Quat quaternion;
    float radius;
};

std::vector<BodyInitialState> g_initial_body_states;

struct PhysicsStats {
    double total_simulation_time = 0.0;
    uint64_t total_steps = 0;
};

PhysicsStats g_physics_stats;

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

    // Physics control
    bool physics_paused = false;
    bool step_once = false;
    float time_scale = 1.0f;
    float custom_gravity = -9.81f;

    // UI state
    bool show_controls = true;
    bool show_body_info = true;
    bool show_stats = true;
    int selected_body_index = 0;
};

// Helper function to reset simulation to initial state
void reset_simulation() {
    int index = 0;
    for (auto bodyIt = g_world->begin(); bodyIt != g_world->end(); ++bodyIt) {
        pe::BodyID body = *bodyIt;
        if (body->getType() == pe::sphereType && index < static_cast<int>(g_initial_body_states.size())) {
            pe::SphereID sphere = pe::static_body_cast<pe::Sphere>(body);
            auto& state = g_initial_body_states[index];

            sphere->setPosition(state.position);
            sphere->setLinearVel(state.velocity);
            sphere->setAngularVel(state.angular_velocity);
            sphere->setOrientation(state.quaternion);
            index++;
        }
    }
    g_physics_stats.total_steps = 0;
    g_physics_stats.total_simulation_time = 0.0;
}

// Helper function to spawn a random sphere
void spawn_random_sphere() {
    static unsigned int spawn_id = 1000;
    static pe::MaterialID material = pe::createMaterial("spawned", 1.0, 0.3, 0.5, 0.05, 0.2, 80, 100, 10, 11);

    float x = (rand() % 400 - 200) / 100.0f;  // -2.0 to 2.0
    float y = 5.0f + (rand() % 300) / 100.0f;  // 5.0 to 8.0
    float z = (rand() % 400 - 200) / 100.0f;
    float radius = 0.3f + (rand() % 40) / 100.0f;  // 0.3 to 0.7

    pe::createSphere(spawn_id++, pe::Vec3(x, y, z), radius, material);
}

void main_loop(void* data)
{
    auto context = static_cast<context_t*>(data);

    // Fixed timestep with accumulator for physics
    static Uint64 last_time = SDL_GetPerformanceCounter();
    Uint64 now = SDL_GetPerformanceCounter();
    double elapsed = (now - last_time) / (double)SDL_GetPerformanceFrequency();
    last_time = now;

    static double accumulator = 0.0;
    const pe::real dt = 1.0 / 60.0;

    // Only accumulate time when not paused
    if (!context->physics_paused) {
        accumulator += elapsed;
    }

    // Step physics in fixed increments (with pause and time scale)
    if (!context->physics_paused) {
        const pe::real scaled_dt = dt * context->time_scale;

        while (accumulator >= dt) {
            g_world->simulationStep(scaled_dt);
            accumulator -= dt;

            g_physics_stats.total_steps++;
            g_physics_stats.total_simulation_time += scaled_dt;
        }
    }

    // Handle single step when paused
    if (context->step_once) {
        const pe::real scaled_dt = dt * context->time_scale;
        g_world->simulationStep(scaled_dt);
        g_physics_stats.total_steps++;
        g_physics_stats.total_simulation_time += scaled_dt;

        context->step_once = false;
        context->physics_paused = true;
    }

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

    // ======= UI Window 1: Physics Controls =======
    if (context->show_controls) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 280), ImGuiCond_FirstUseEver);
        ImGui::Begin("Physics Controls", &context->show_controls);

        // Pause/Play toggle
        if (context->physics_paused) {
            if (ImGui::Button("Play", ImVec2(120, 30))) {
                context->physics_paused = false;
            }
        } else {
            if (ImGui::Button("|| Pause", ImVec2(120, 30))) {
                context->physics_paused = true;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(120, 30))) {
            reset_simulation();
            context->physics_paused = false;
        }

        // Single step (only when paused)
        ImGui::BeginDisabled(!context->physics_paused);
        if (ImGui::Button("Step Frame", ImVec2(250, 25))) {
            context->step_once = true;
        }
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Text("Time Control");
        ImGui::SliderFloat("Time Scale", &context->time_scale, 0.1f, 2.0f, "%.1fx");

        ImGui::Separator();
        ImGui::Text("Environment");
        if (ImGui::SliderFloat("Gravity", &context->custom_gravity, -20.0f, 0.0f, "%.2f m/s^2")) {
            g_world->setGravity(pe::Vec3(0.0, context->custom_gravity, 0.0));
        }

        ImGui::Separator();
        if (ImGui::Button("Spawn Random Sphere", ImVec2(250, 30))) {
            spawn_random_sphere();
        }

        ImGui::End();
    }

    // ======= UI Window 2: Body Inspector =======
    if (context->show_body_info) {
        ImGui::SetNextWindowPos(ImVec2(10, 310), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Body Inspector", &context->show_body_info);

        int num_bodies = static_cast<int>(g_world->size()) - 1;  // Exclude ground plane
        if (num_bodies > 0) {
            ImGui::SliderInt("Body Index", &context->selected_body_index, 0, num_bodies - 1);

            // Iterate to find the Nth sphere (skip plane)
            int sphere_count = 0;
            for (auto bodyIt = g_world->begin(); bodyIt != g_world->end(); ++bodyIt) {
                pe::BodyID body = *bodyIt;
                if (body->getType() == pe::sphereType) {
                    if (sphere_count == context->selected_body_index) {
                        pe::SphereID sphere = pe::static_body_cast<pe::Sphere>(body);

                        const pe::Vec3& pos = sphere->getPosition();
                        const pe::Vec3& vel = sphere->getLinearVel();
                        const pe::Vec3& avel = sphere->getAngularVel();

                        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos[0], pos[1], pos[2]);
                        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", vel[0], vel[1], vel[2]);

                        // Calculate speed
                        float speed = static_cast<float>(std::sqrt(vel[0]*vel[0] + vel[1]*vel[1] + vel[2]*vel[2]));
                        ImGui::Text("Speed: %.2f m/s", speed);
                        ImGui::Text("Angular Vel: (%.2f, %.2f, %.2f)", avel[0], avel[1], avel[2]);

                        ImGui::Separator();

                        float mass = static_cast<float>(sphere->getMass());
                        float radius = static_cast<float>(sphere->getRadius());
                        ImGui::Text("Mass: %.2f kg", mass);
                        ImGui::Text("Radius: %.2f m", radius);

                        // Kinetic energy: KE_linear + KE_rotational
                        float vel_sqr = static_cast<float>(vel[0]*vel[0] + vel[1]*vel[1] + vel[2]*vel[2]);
                        float avel_sqr = static_cast<float>(avel[0]*avel[0] + avel[1]*avel[1] + avel[2]*avel[2]);
                        float ke_linear = 0.5f * mass * vel_sqr;
                        float moment_inertia = (2.0f/5.0f) * mass * radius * radius;  // Sphere: I = (2/5)mr²
                        float ke_angular = 0.5f * moment_inertia * avel_sqr;

                        ImGui::Text("Kinetic Energy: %.2f J", ke_linear + ke_angular);
                        ImGui::Text("  Linear: %.2f J", ke_linear);
                        ImGui::Text("  Angular: %.2f J", ke_angular);

                        break;
                    }
                    sphere_count++;
                }
            }
        } else {
            ImGui::Text("No bodies in simulation");
        }

        ImGui::End();
    }

    // ======= UI Window 3: Simulation Statistics =======
    if (context->show_stats) {
        ImGui::SetNextWindowPos(ImVec2(context->width - 310, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Simulation Stats", &context->show_stats);

        ImGui::Text("Bodies: %lu", g_world->size());
        ImGui::Text("Simulation Steps: %lu", g_physics_stats.total_steps);
        ImGui::Text("Simulation Time: %.2f s", g_physics_stats.total_simulation_time);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::Separator();
        ImGui::Text("System Totals");

        // Calculate total kinetic energy and momentum
        float total_ke = 0.0f;
        pe::Vec3 total_momentum(0, 0, 0);

        for (auto bodyIt = g_world->begin(); bodyIt != g_world->end(); ++bodyIt) {
            pe::BodyID body = *bodyIt;
            if (body->getType() == pe::sphereType) {
                pe::SphereID sphere = pe::static_body_cast<pe::Sphere>(body);
                const pe::Vec3& vel = sphere->getLinearVel();
                const pe::Vec3& avel = sphere->getAngularVel();
                float mass = static_cast<float>(sphere->getMass());
                float radius = static_cast<float>(sphere->getRadius());

                // Linear + rotational kinetic energy
                float vel_sqr = static_cast<float>(vel[0]*vel[0] + vel[1]*vel[1] + vel[2]*vel[2]);
                float avel_sqr = static_cast<float>(avel[0]*avel[0] + avel[1]*avel[1] + avel[2]*avel[2]);
                float ke_linear = 0.5f * mass * vel_sqr;
                float moment = (2.0f/5.0f) * mass * radius * radius;
                float ke_angular = 0.5f * moment * avel_sqr;
                total_ke += ke_linear + ke_angular;

                total_momentum += vel * mass;
            }
        }

        ImGui::Text("Total KE: %.2f J", total_ke);
        ImGui::Text("Momentum: (%.1f, %.1f, %.1f)",
                    total_momentum[0], total_momentum[1], total_momentum[2]);

        ImGui::End();
    }

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

    // Debug draw for grid only
    DebugDrawEncoder dde;
    dde.begin(0);

    // Draw ground grid
    bx::Vec3 up = { 0.0f, 1.0f, 0.0f };
    bx::Vec3 origin = { 0.0f, 0.0f, 0.0f };
    dde.setColor(0xff404040);
    dde.drawGrid(up, origin, 20, 1.0f);

    dde.end();

    // Build sphere instances from physics bodies
    std::vector<SphereInstance> sphere_instances;
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

            // Select color (convert from ABGR uint32 to RGBA float)
            uint32_t color = SPHERE_COLORS[sphereIndex % NUM_COLORS];
            float r = float((color >> 0) & 0xFF) / 255.0f;
            float g = float((color >> 8) & 0xFF) / 255.0f;
            float b = float((color >> 16) & 0xFF) / 255.0f;
            float a = float((color >> 24) & 0xFF) / 255.0f;

            // Build instance
            SphereInstance inst;

            // Build transform matrix: scale by radius, apply rotation, translate
            // PE rotation matrix is 3x3 column-major: columns are [0,3,6], [1,4,7], [2,5,8]
            // bgfx expects column-major 4x4

            // Column 0 (X axis scaled by radius)
            inst.transform[0] = static_cast<float>(rot[0]) * radius;
            inst.transform[1] = static_cast<float>(rot[3]) * radius;
            inst.transform[2] = static_cast<float>(rot[6]) * radius;
            inst.transform[3] = 0.0f;

            // Column 1 (Y axis scaled by radius)
            inst.transform[4] = static_cast<float>(rot[1]) * radius;
            inst.transform[5] = static_cast<float>(rot[4]) * radius;
            inst.transform[6] = static_cast<float>(rot[7]) * radius;
            inst.transform[7] = 0.0f;

            // Column 2 (Z axis scaled by radius)
            inst.transform[8] = static_cast<float>(rot[2]) * radius;
            inst.transform[9] = static_cast<float>(rot[5]) * radius;
            inst.transform[10] = static_cast<float>(rot[8]) * radius;
            inst.transform[11] = 0.0f;

            // Column 3 (translation)
            inst.transform[12] = static_cast<float>(pos[0]);
            inst.transform[13] = static_cast<float>(pos[1]);
            inst.transform[14] = static_cast<float>(pos[2]);
            inst.transform[15] = 1.0f;

            // Color
            inst.color[0] = r;
            inst.color[1] = g;
            inst.color[2] = b;
            inst.color[3] = a;

            sphere_instances.push_back(inst);
            sphereIndex++;
        }
    }

    // Render all spheres in a single draw call
    if (!sphere_instances.empty()) {
        g_sphere_renderer.render(0, sphere_instances.data(), uint32_t(sphere_instances.size()));
    }

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
    // Position window on the right side (offset from left edge to avoid center gap on dual monitors)
    SDL_Window* window = SDL_CreateWindow(
        "00-physics-basic", -430, 0,
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

    //jk
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

    // Initialize sphere renderer
    SphereRendererConfig sphere_config;
    sphere_config.segments = 24;
    sphere_config.rings = 16;
    sphere_config.texture_size = 64;
    sphere_config.checker_size = 8;
    if (!g_sphere_renderer.init(sphere_config)) {
        printf("Failed to initialize sphere renderer\n");
        return 1;
    }

    // Initialize PE physics world
    g_world = pe::theWorld();
    g_world->setGravity(pe::Vec3(0.0, -9.81, 0.0));

    // Enable adaptive Baumgarte stabilization capping (off by default)
    // This prevents "explosions" from deep penetrations with small timesteps
    pe::theCollisionSystem()->setAdaptiveBaumgarteCapping(true, 50.0);

    unsigned int idx = 0;

    // Create material
    pe::MaterialID material = pe::createMaterial("default", 1.0, 0.3, 0.5, 0.05, 0.2, 80, 100, 10, 11);

    // Create ground plane
    g_ground = pe::createPlane(idx++, 0.0, 1.0, 0.0, 0.0, material);

    // Create some spheres
    pe::SphereID sphere1 = pe::createSphere(idx++, pe::Vec3(0.0, 5.0, 0.0), 0.5, material);

    // Capture initial state for sphere1
    BodyInitialState state1;
    state1.position = sphere1->getPosition();
    state1.velocity = sphere1->getLinearVel();
    state1.angular_velocity = sphere1->getAngularVel();
    state1.quaternion = sphere1->getQuaternion();
    state1.radius = static_cast<float>(sphere1->getRadius());
    g_initial_body_states.push_back(state1);

    pe::SphereID sphere2 = pe::createSphere(idx++, pe::Vec3(1.5, 6.0, 0.0), 0.4, material);

    // Capture initial state for sphere2
    BodyInitialState state2;
    state2.position = sphere2->getPosition();
    state2.velocity = sphere2->getLinearVel();
    state2.angular_velocity = sphere2->getAngularVel();
    state2.quaternion = sphere2->getQuaternion();
    state2.radius = static_cast<float>(sphere2->getRadius());
    g_initial_body_states.push_back(state2);

    pe::SphereID sphere3 = pe::createSphere(idx++, pe::Vec3(-1.5, 7.0, 0.0), 0.6, material);
    sphere3->setAngularVel(pe::Vec3(0,0,-6.28/6.0));

    // Capture initial state for sphere3
    BodyInitialState state3;
    state3.position = sphere3->getPosition();
    state3.velocity = sphere3->getLinearVel();
    state3.angular_velocity = sphere3->getAngularVel();
    state3.quaternion = sphere3->getQuaternion();
    state3.radius = static_cast<float>(sphere3->getRadius());
    g_initial_body_states.push_back(state3);

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
    g_sphere_renderer.shutdown();
    ddShutdown();

    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();
    ImGui::DestroyContext();

    bgfx::shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
