#include "sphere_renderer.h"
#include "sphere_mesh.h"

#include <bx/math.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

namespace {

// Load shader binary from file
bgfx::ShaderHandle load_shader(const char* path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        printf("Failed to open shader: %s\n", path);
        return BGFX_INVALID_HANDLE;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    const bgfx::Memory* mem = bgfx::alloc(uint32_t(size + 1));
    if (!file.read(reinterpret_cast<char*>(mem->data), size)) {
        printf("Failed to read shader: %s\n", path);
        return BGFX_INVALID_HANDLE;
    }
    mem->data[size] = '\0';

    return bgfx::createShader(mem);
}

} // anonymous namespace

bool SphereRenderer::init(const SphereRendererConfig& config)
{
    if (m_initialized) {
        return true;
    }

    // Setup vertex layout
    m_vertex_layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    // Create mesh
    create_mesh(config.segments, config.rings);

    // Create checkerboard texture
    create_checkerboard_texture(config.texture_size, config.checker_size);

    // Load shaders
    bgfx::ShaderHandle vsh = load_shader("shader/build/vs_sphere_instanced.bin");
    bgfx::ShaderHandle fsh = load_shader("shader/build/fs_sphere_textured.bin");

    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        printf("Failed to load sphere shaders\n");
        return false;
    }

    m_program = bgfx::createProgram(vsh, fsh, true);
    if (!bgfx::isValid(m_program)) {
        printf("Failed to create sphere program\n");
        return false;
    }

    // Create uniforms
    m_sampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
    m_light_dir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);

    m_initialized = true;
    return true;
}

void SphereRenderer::create_mesh(uint32_t segments, uint32_t rings)
{
    // Destroy existing buffers if any
    if (bgfx::isValid(m_vbh)) {
        bgfx::destroy(m_vbh);
    }
    if (bgfx::isValid(m_ibh)) {
        bgfx::destroy(m_ibh);
    }

    m_segments = segments;
    m_rings = rings;

    // Generate sphere mesh
    sphere_mesh::SphereMesh mesh = sphere_mesh::generate_uv_sphere(segments, rings);

    // Create vertex buffer
    const bgfx::Memory* vb_mem = bgfx::copy(
        mesh.vertices.data(),
        uint32_t(mesh.vertices.size() * sizeof(sphere_mesh::SphereVertex))
    );
    m_vbh = bgfx::createVertexBuffer(vb_mem, m_vertex_layout);

    // Create index buffer
    const bgfx::Memory* ib_mem = bgfx::copy(
        mesh.indices.data(),
        uint32_t(mesh.indices.size() * sizeof(uint16_t))
    );
    m_ibh = bgfx::createIndexBuffer(ib_mem);

    m_index_count = uint32_t(mesh.indices.size());
}

void SphereRenderer::create_checkerboard_texture(uint32_t size, uint32_t checker_size)
{
    if (bgfx::isValid(m_texture)) {
        bgfx::destroy(m_texture);
    }

    std::vector<uint32_t> pixels(size * size);

    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            uint32_t cx = x / checker_size;
            uint32_t cy = y / checker_size;
            bool white = ((cx + cy) % 2) == 0;

            // RGBA format (ABGR in memory)
            pixels[y * size + x] = white ? 0xFFFFFFFF : 0xFF808080;
        }
    }

    const bgfx::Memory* mem = bgfx::copy(pixels.data(), uint32_t(pixels.size() * sizeof(uint32_t)));
    m_texture = bgfx::createTexture2D(
        uint16_t(size), uint16_t(size),
        false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
        mem
    );
}

void SphereRenderer::render(bgfx::ViewId view_id, const SphereInstance* instances, uint32_t count)
{
    if (!m_initialized || count == 0) {
        return;
    }

    // Instance data stride: 16 floats for transform + 4 floats for color = 80 bytes
    const uint16_t instance_stride = 80;

    // Check how many instances we can fit
    uint32_t available = bgfx::getAvailInstanceDataBuffer(count, instance_stride);
    if (available == 0) {
        return;
    }
    uint32_t to_render = (available < count) ? available : count;

    // Allocate instance data buffer
    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, to_render, instance_stride);

    // Fill instance data
    uint8_t* data = idb.data;
    for (uint32_t i = 0; i < to_render; ++i) {
        const SphereInstance& inst = instances[i];

        // Copy transform (64 bytes)
        std::memcpy(data, inst.transform, 16 * sizeof(float));
        data += 16 * sizeof(float);

        // Copy color (16 bytes)
        std::memcpy(data, inst.color, 4 * sizeof(float));
        data += 4 * sizeof(float);
    }

    // Set render state
    uint64_t state = BGFX_STATE_WRITE_RGB
                   | BGFX_STATE_WRITE_A
                   | BGFX_STATE_WRITE_Z
                   | BGFX_STATE_DEPTH_TEST_LESS
                   | BGFX_STATE_CULL_CCW
                   | BGFX_STATE_MSAA;

    bgfx::setState(state);

    // Set vertex and index buffers
    bgfx::setVertexBuffer(0, m_vbh);
    bgfx::setIndexBuffer(m_ibh);

    // Set instance data
    bgfx::setInstanceDataBuffer(&idb);

    // Set texture
    bgfx::setTexture(0, m_sampler, m_texture);

    // Set light direction uniform (normalized direction toward light)
    float light_dir[4] = { 0.5f, 0.8f, 0.3f, 0.0f };
    bx::Vec3 light_vec = bx::normalize(bx::Vec3(light_dir[0], light_dir[1], light_dir[2]));
    light_dir[0] = light_vec.x;
    light_dir[1] = light_vec.y;
    light_dir[2] = light_vec.z;
    bgfx::setUniform(m_light_dir, light_dir);

    // Submit draw call
    bgfx::submit(view_id, m_program);
}

void SphereRenderer::set_resolution(uint32_t segments, uint32_t rings)
{
    if (!m_initialized) {
        return;
    }

    if (segments != m_segments || rings != m_rings) {
        create_mesh(segments, rings);
    }
}

void SphereRenderer::shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (bgfx::isValid(m_vbh)) {
        bgfx::destroy(m_vbh);
        m_vbh = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_ibh)) {
        bgfx::destroy(m_ibh);
        m_ibh = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_texture)) {
        bgfx::destroy(m_texture);
        m_texture = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_sampler)) {
        bgfx::destroy(m_sampler);
        m_sampler = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_light_dir)) {
        bgfx::destroy(m_light_dir);
        m_light_dir = BGFX_INVALID_HANDLE;
    }

    m_initialized = false;
}
