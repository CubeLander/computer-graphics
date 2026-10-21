#pragma once
#include "orbits.hpp"
#include <array>
#include <stdexcept>
#include <vector>

// Normal/UV default to zero for lines and HUD geometry (unlit shader path).
struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 normal{};
    glm::vec2 uv{};
};

inline std::vector<Vertex> makeSphere(int slices = 128, int stacks = 64) {
    if (slices < 3 || stacks < 2) throw std::invalid_argument("Sphere needs at least 3 slices and 2 stacks");
    std::vector<Vertex> vertices;
    vertices.reserve(size_t(slices) * stacks * 6);
    for (int j = 0; j < stacks; ++j) for (int i = 0; i < slices; ++i) {
        float shade = (i * 12 / slices) % 2 ? 0.83f : 1.0f;
        if (i == 0 || j == stacks / 2) shade = 1.3f;
        for (auto ij : std::array<glm::ivec2,6>{{{i,j},{i,j+1},{i+1,j+1},{i,j},{i+1,j+1},{i+1,j}}}) {
            glm::vec2 uv(float(ij.x) / slices, float(ij.y) / stacks);
            float colatitude = float(orbit::pi) * uv.y;
            float longitude = 2 * float(orbit::pi) * uv.x;
            glm::vec3 p(std::sin(colatitude) * std::cos(longitude), std::cos(colatitude),
                        -std::sin(colatitude) * std::sin(longitude));
            // Unindexed triangles duplicate the longitude seam: each triangle's
            // U span stays small, never interpolating from U=1 back through U=0.
            // JPEG first row is north, so V=0 is north; do not flip image rows.
            vertices.push_back({p, {shade,shade,shade,1}, p, uv});
        }
    }
    return vertices;
}
