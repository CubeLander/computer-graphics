#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex { glm::vec3 position; glm::vec4 color; };
struct Mesh { unsigned vao = 0, vbo = 0; int count = 0; unsigned mode = 0; };
// Owns GPU resources; must be destroyed before the window/context.
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void scene(double days, const glm::mat4& vp, bool planes);
    void text(float x, float y, float scale, const std::string& value, glm::vec4 color,
              int width, int height);
    void panel(float x, float y, float w, float h, glm::vec4 color, int width, int height);
    void screenshot(const std::string& path, int width, int height);
private:
    unsigned program = 0;
    int transformLocation = -1, tintLocation = -1;
    Mesh sphere, ring, disk, stars, overlay;
    Mesh upload(const std::vector<Vertex>& data, unsigned mode);
    void draw(const Mesh& mesh, const glm::mat4& transform, glm::vec4 tint);
    void overlayDraw(const std::vector<Vertex>& data, int width, int height);
};
