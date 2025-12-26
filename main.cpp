#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <debugdraw/debugdraw.h>

#include <SDL.h>
#include <SDL_syswm.h>

// PE requires config.h to be included first for PE_PUBLIC and other macros
#include <config.h>
#include <pe/core.h>
#include <pe/core/Types.h>

#include "bgfx-imgui/imgui_impl_bgfx.h"
#include "file-ops.h"
#include "imgui.h"
#include "sdl-imgui/imgui_impl_sdl2.h"

#if BX_PLATFORM_EMSCRIPTEN
#include "emscripten.h"
#endif // BX_PLATFORM_EMSCRIPTEN


pe::WorldID g_world_id;
pe::SphereID g_sphere;
pe::PlaneID g_plane;

struct PosColorVertex
{
    float x;
    float y;
    float z;
    uint32_t abgr;
};

static PosColorVertex cube_vertices[] = {
    {-1.0f, 1.0f, 1.0f, 0xff000000},   {1.0f, 1.0f, 1.0f, 0xff0000ff},
    {-1.0f, -1.0f, 1.0f, 0xff00ff00},  {1.0f, -1.0f, 1.0f, 0xff00ffff},
    {-1.0f, 1.0f, -1.0f, 0xffff0000},  {1.0f, 1.0f, -1.0f, 0xffff00ff},
    {-1.0f, -1.0f, -1.0f, 0xffffff00}, {1.0f, -1.0f, -1.0f, 0xffffffff},
};

static const uint16_t cube_tri_list[] = {
    0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6,
    1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
};

static bgfx::ShaderHandle create_shader(
    const std::string& shader, const char* name)
{
    const bgfx::Memory* mem = bgfx::copy(shader.data(), shader.size());
    const bgfx::ShaderHandle handle = bgfx::createShader(mem);
    bgfx::setName(handle, name);
    return handle;
}

struct context_t
{
    SDL_Window* window = nullptr;
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;

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

    // start with the physics step
    pe::real dt = (1.0/ 60.0);
    g_world_id->simulationStep(dt);

    auto context = static_cast<context_t*>(data);

    for (SDL_Event current_event; SDL_PollEvent(&current_event) != 0;) {
        ImGui_ImplSDL2_ProcessEvent(&current_event);
        if (current_event.type == SDL_QUIT) {
            context->quit = true;
            break;
        }
    }

    ImGui_Implbgfx_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();
    ImGui::ShowDemoWindow(); // your drawing here
    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

    if (!ImGui::GetIO().WantCaptureMouse) {
        // simple input code for orbit camera
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

    float cam_rotation[16];
    bx::mtxRotateXYZ(cam_rotation, context->cam_pitch, context->cam_yaw, 0.0f);

    float cam_translation[16];
    bx::mtxTranslate(cam_translation, 0.0f, 1.0f, -5.0f);

    float cam_transform[16];
    bx::mtxMul(cam_transform, cam_translation, cam_rotation);

    float view[16];
    bx::mtxInverse(view, cam_transform);

    float proj[16];
    bx::mtxProj(
        proj, 60.0f, float(context->width) / float(context->height), 0.1f,
        100.0f, bgfx::getCaps()->homogeneousDepth);

    bgfx::setViewTransform(0, view, proj);

    float model[16];

    // Add at top of main.cpp (after includes, before main)
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

    // Debug draw test
    DebugDrawEncoder dde;
    dde.begin(0);

    int sphereIndex = 0;
    // Instead of hardcoding g_sphere
    for (auto bodyIt = g_world_id->begin(); bodyIt != g_world_id->end(); ++bodyIt)
    {
        pe::BodyID body = *bodyIt;
        if (body->getType() == pe::sphereType) {  // Skip infinite bodies (planes)
            // Render based on type
            // Draw a test sphere
            bx::Sphere testSphere;

            pe::Vec3 pos = body->getPosition();
            testSphere.center = { pos[0], pos[1], pos[2] };
            pe::SphereID s = pe::static_body_cast<pe::Sphere>(body);
            pe::real rad = s->getRadius();            

            // Select color
            uint32_t color = SPHERE_COLORS[sphereIndex % NUM_COLORS];

            testSphere.radius = static_cast<float>(rad);
            dde.setColor(color);  // Green
            dde.draw(testSphere);

            // Draw orientation axes to show rotation
            pe::Rot3 rot = body->getRotation();
            float axisLength = testSphere.radius * 1.2f;
        
            dde.push();
                // X-axis (red)
                dde.setColor(0xff0000ff);
                dde.moveTo(pos[0], pos[1], pos[2]);
                dde.lineTo(
                    pos[0] + rot[0] * axisLength,
                    pos[1] + rot[3] * axisLength,
                    pos[2] + rot[6] * axisLength
                );
        
                // Y-axis (green)
                dde.setColor(0xff00ff00);
                dde.moveTo(pos[0], pos[1], pos[2]);
                dde.lineTo(
                    pos[0] + rot[1] * axisLength,
                    pos[1] + rot[4] * axisLength,
                    pos[2] + rot[7] * axisLength
                );
        
                // Z-axis (blue)
                dde.setColor(0xffff0000);
                dde.moveTo(pos[0], pos[1], pos[2]);
                dde.lineTo(
                    pos[0] + rot[2] * axisLength,
                    pos[1] + rot[5] * axisLength,
                    pos[2] + rot[8] * axisLength
                );
            dde.pop();    
            sphereIndex++;
        }
    }    


    // Draw ground grid
    bx::Vec3 up = { 0.0f, 1.0f, 0.0f };
    bx::Vec3 origin = { 0.0f, 0.0f, 0.0f };
    dde.setColor(0xff404040);  // Dark gray
    dde.drawGrid(up, origin, 10, 1.0f);

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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    const int width = 800;
    const int height = 600;
    SDL_Window* window = SDL_CreateWindow(
        argv[0], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
        height, SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        printf("Window could not be created. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

#if !BX_PLATFORM_EMSCRIPTEN
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        printf(
            "SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
            SDL_GetError());
        return 1;
    }
    bgfx::renderFrame(); // single threaded mode
#endif // !BX_PLATFORM_EMSCRIPTEN

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
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
       // BX_PLATFORM_EMSCRIPTEN

    bgfx::Init bgfx_init;
    bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
    bgfx_init.resolution.width = width;
    bgfx_init.resolution.height = height;
    bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx_init.platformData = pd;
    bgfx::init(bgfx_init);

    bgfx::setViewClear(
        0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width, height);

    ImGui::CreateContext();

    ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
    ImGui_ImplSDL2_InitForD3D(window);
#elif BX_PLATFORM_OSX
    ImGui_ImplSDL2_InitForMetal(window);
#elif BX_PLATFORM_LINUX || BX_PLATFORM_EMSCRIPTEN
    ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
       // BX_PLATFORM_EMSCRIPTEN

    // Initialize debug draw
    ddInit();

    g_world_id = pe::theWorld();
    g_world_id->setGravity(pe::Vec3(0.0, -0.4, 0.0));
    unsigned int idx = 0;
    pe::MaterialID myMaterial = pe::createMaterial("test", 1.0, 0.2, 0.5, 0.05, 0.2, 80, 100, 10, 11); 
    g_sphere = pe::createSphere(idx++, pe::Vec3(0.4, 2.0, 2.0), 0.5, myMaterial);
    pe::createSphere(idx++, pe::Vec3(0.0, 4.0, 2.0), 0.5, myMaterial);
     
    // Setup of the ground plane
    //g_plane = pe::createPlane( idx++, 0.0, 1.0, 0.0, -0.0, pe::granite );
    g_plane = pe::createPlane( idx++, 0.0, 1.0, 0.0, -0.0, myMaterial );

    const std::string shader_root =
#if BX_PLATFORM_EMSCRIPTEN
        "shader/embuild/";
#else
        "shader/build/";
#endif // BX_PLATFORM_EMSCRIPTEN

    std::string vshader;
    if (!fileops::read_file(shader_root + "v_simple.bin", vshader)) {
        printf("Could not find shader vertex shader (ensure shaders have been "
               "compiled).\n"
               "Run compile-shaders-<platform>.sh/bat\n");
        return 1;
    }

    std::string fshader;
    if (!fileops::read_file(shader_root + "f_simple.bin", fshader)) {
        printf("Could not find shader fragment shader (ensure shaders have "
               "been compiled).\n"
               "Run compile-shaders-<platform>.sh/bat\n");
        return 1;
    }

    bgfx::ShaderHandle vsh = create_shader(vshader, "vshader");
    bgfx::ShaderHandle fsh = create_shader(fshader, "fshader");
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);

    context_t context;
    context.width = width;
    context.height = height;
    context.program = program;
    context.window = window;

#if BX_PLATFORM_EMSCRIPTEN
    emscripten_set_main_loop_arg(main_loop, &context, -1, 1);
#else
    while (!context.quit) {
        main_loop(&context);
    }
#endif // BX_PLATFORM_EMSCRIPTEN

    bgfx::destroy(program);

    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();

    ImGui::DestroyContext();

    // Shutdown debug draw
    ddShutdown();

    bgfx::shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
