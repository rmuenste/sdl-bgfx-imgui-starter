#include "sphere_mesh.h"
#include <cmath>

namespace sphere_mesh {

SphereMesh generate_uv_sphere(uint32_t segments, uint32_t rings)
{
    SphereMesh mesh;

    const float PI = 3.14159265358979323846f;

    // Generate vertices
    // We need (segments + 1) vertices around for proper UV wrapping
    // We need (rings + 1) rows of vertices from north pole to south pole
    mesh.vertices.reserve((segments + 1) * (rings + 1));

    for (uint32_t ring = 0; ring <= rings; ++ring) {
        // phi goes from 0 (north pole) to PI (south pole)
        float phi = PI * float(ring) / float(rings);
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);

        for (uint32_t seg = 0; seg <= segments; ++seg) {
            // theta goes from 0 to 2*PI around the sphere
            float theta = 2.0f * PI * float(seg) / float(segments);
            float sin_theta = std::sin(theta);
            float cos_theta = std::cos(theta);

            SphereVertex v;

            // Position on unit sphere
            v.position[0] = sin_phi * cos_theta;  // x
            v.position[1] = cos_phi;               // y (up)
            v.position[2] = sin_phi * sin_theta;  // z

            // Normal is same as position for unit sphere
            v.normal[0] = v.position[0];
            v.normal[1] = v.position[1];
            v.normal[2] = v.position[2];

            // UV coordinates
            v.texcoord[0] = float(seg) / float(segments);
            v.texcoord[1] = float(ring) / float(rings);

            mesh.vertices.push_back(v);
        }
    }

    // Generate indices
    // Each quad is made of 2 triangles
    mesh.indices.reserve(segments * rings * 6);

    for (uint32_t ring = 0; ring < rings; ++ring) {
        for (uint32_t seg = 0; seg < segments; ++seg) {
            // Vertex indices for current quad
            uint16_t top_left = ring * (segments + 1) + seg;
            uint16_t top_right = top_left + 1;
            uint16_t bottom_left = (ring + 1) * (segments + 1) + seg;
            uint16_t bottom_right = bottom_left + 1;

            // First triangle (top-left, bottom-left, bottom-right)
            mesh.indices.push_back(top_left);
            mesh.indices.push_back(bottom_left);
            mesh.indices.push_back(bottom_right);

            // Second triangle (top-left, bottom-right, top-right)
            mesh.indices.push_back(top_left);
            mesh.indices.push_back(bottom_right);
            mesh.indices.push_back(top_right);
        }
    }

    return mesh;
}

} // namespace sphere_mesh
