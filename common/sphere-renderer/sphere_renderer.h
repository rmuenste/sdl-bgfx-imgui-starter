#pragma once

#include <bgfx/bgfx.h>
#include <cstdint>

// Instance data for a single sphere
struct SphereInstance {
    float transform[16];  // 4x4 matrix (scale * rotation * translation)
    float color[4];       // RGBA (0.0 - 1.0)
};

// Configuration for sphere renderer
struct SphereRendererConfig {
    uint32_t segments = 16;       // Longitude divisions
    uint32_t rings = 12;          // Latitude divisions
    uint32_t texture_size = 64;   // Checkerboard texture size
    uint32_t checker_size = 8;    // Size of each checker square
};

class SphereRenderer {
public:
    SphereRenderer() = default;
    ~SphereRenderer() = default;

    // Initialize the renderer with the given configuration
    bool init(const SphereRendererConfig& config = SphereRendererConfig{});

    // Render all instances in a single draw call
    void render(bgfx::ViewId view_id, const SphereInstance* instances, uint32_t count);

    // Rebuild the mesh with new resolution
    void set_resolution(uint32_t segments, uint32_t rings);

    // Clean up all resources
    void shutdown();

    // Get current resolution
    uint32_t get_segments() const { return m_segments; }
    uint32_t get_rings() const { return m_rings; }

private:
    void create_mesh(uint32_t segments, uint32_t rings);
    void create_checkerboard_texture(uint32_t size, uint32_t checker_size);

    bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_ibh = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_texture = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sampler = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_light_dir = BGFX_INVALID_HANDLE;

    bgfx::VertexLayout m_vertex_layout;

    uint32_t m_segments = 0;
    uint32_t m_rings = 0;
    uint32_t m_index_count = 0;

    bool m_initialized = false;
};
