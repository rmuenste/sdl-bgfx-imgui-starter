# Common library for bgfx applications
# Provides shared code used by all examples and main executable:
# - ImGui SDL2 implementation
# - ImGui bgfx implementation
# - Debug draw functionality

add_library(bgfx-common STATIC)

target_sources(bgfx-common PRIVATE
    sdl-imgui/imgui_impl_sdl2.cpp
    bgfx-imgui/imgui_impl_bgfx.cpp
    sphere-renderer/sphere_mesh.cpp
    sphere-renderer/sphere_renderer.cpp
    ${CMAKE_SOURCE_DIR}/third-party/build/src/bgfx/bgfx/examples/common/debugdraw/debugdraw.cpp
)

target_include_directories(bgfx-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third-party/build/src/bgfx/bgfx/examples/common
)

target_link_libraries(bgfx-common PUBLIC
    SDL2::SDL2-static
    bgfx::bgfx
    bgfx::bx
    imgui.cmake::imgui.cmake
)

target_compile_features(bgfx-common PUBLIC cxx_std_11)
