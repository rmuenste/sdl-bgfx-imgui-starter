#pragma once

#include <cstdint>
#include <vector>

namespace sphere_mesh {

struct SphereVertex {
    float position[3];
    float normal[3];
    float texcoord[2];
};

struct SphereMesh {
    std::vector<SphereVertex> vertices;
    std::vector<uint16_t> indices;
};

// Generate a UV sphere with the specified number of segments (longitude) and rings (latitude)
// segments: divisions around the equator (longitude lines)
// rings: divisions from pole to pole (latitude lines)
// Vertex count: (segments + 1) * (rings + 1)
// Index count: segments * rings * 6
SphereMesh generate_uv_sphere(uint32_t segments, uint32_t rings);

} // namespace sphere_mesh
