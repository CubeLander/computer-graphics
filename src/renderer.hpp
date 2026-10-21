#pragma once
#include <glm/glm.hpp>
#include "sphere.hpp"
#include <string>
#include <vector>

struct Mesh { unsigned vao = 0, vbo = 0; int count = 0; unsigned mode = 0; };
// Owns GPU resources; must be destroyed before the window/context.
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void scene(double days, const glm::mat4& vp, glm::vec3 eye, bool planes, bool textured);
    void text(float x, float y, float scale, const std::string& value, glm::vec4 color,
              int width, int height);
    void panel(float x, float y, float w, float h, glm::vec4 color, int width, int height);
    void screenshot(const std::string& path, int width, int height);
private:
    unsigned program = 0;
    int transformLocation = -1, tintLocation = -1;
    Mesh sphere, ring, disk, stars, overlay;
    unsigned earthTexture = 0, moonTexture = 0, sunTexture = 0;
    int modelLocation = -1, normalLocation = -1, eyeLocation = -1;
    int surfaceLocation = -1, specularLocation = -1;
    void release();
    Mesh upload(const std::vector<Vertex>& data, unsigned mode);
    void draw(const Mesh& mesh, const glm::mat4& transform, glm::vec4 tint,
              const glm::mat4& model = glm::mat4(1), int surface = 0,
              unsigned texture = 0, float specular = 0);
    void overlayDraw(const std::vector<Vertex>& data, int width, int height);
};
