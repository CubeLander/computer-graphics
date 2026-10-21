#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define STB_EASY_FONT_IMPLEMENTATION
#include <stb_easy_font.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include "renderer.hpp"
#include "orbits.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <iostream>

namespace {
unsigned compile(unsigned type, const char* source) {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int ok; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096]; glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compilation: ") + log);
    }
    return shader;
}
glm::mat4 model(glm::vec3 position, float radius, glm::mat4 orientation = glm::mat4(1)) {
    return glm::scale(glm::translate(glm::mat4(1), position) * orientation, glm::vec3(radius));
}
}

Mesh Renderer::upload(const std::vector<Vertex>& data, unsigned mode) {
    Mesh m; m.count = static_cast<int>(data.size()); m.mode = mode;
    glGenVertexArrays(1, &m.vao); glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao); glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
    return m;
}
Renderer::Renderer() {
    const char* vs = R"(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
uniform mat4 transform;
uniform vec4 tint;
out vec4 vertexColor;
void main() { gl_Position = transform * vec4(position,1); vertexColor = color * tint; }
)";
    const char* fs = R"(#version 330 core
in vec4 vertexColor;
out vec4 fragColor;
void main() { fragColor = vertexColor; }
)";
    unsigned v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
    program = glCreateProgram(); glAttachShader(program, v); glAttachShader(program, f);
    glLinkProgram(program); glDeleteShader(v); glDeleteShader(f);
    int ok; glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096]; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program); throw std::runtime_error(std::string("Shader link: ") + log);
    }
    transformLocation = glGetUniformLocation(program, "transform");
    tintLocation = glGetUniformLocation(program, "tint");
    std::vector<Vertex> vertices;
    // Alternating longitude sectors make axial rotation visible without textures or lighting.
    constexpr int slices = 96, stacks = 48;
    auto point = [](int i, int j) {
        float lat = float(orbit::pi) * float(j) / stacks;
        float lon = 2 * float(orbit::pi) * float(i) / slices;
        return glm::vec3(std::sin(lat)*std::cos(lon), std::cos(lat), -std::sin(lat)*std::sin(lon));
    };
    for (int j = 0; j < stacks; ++j) for (int i = 0; i < slices; ++i) {
        float shade = (i / 8) % 2 ? 0.83f : 1.0f;
        if (i == 0 || j == stacks/2) shade = 1.3f;
        glm::vec4 color(shade, shade, shade, 1);
        for (auto ij : std::array<glm::ivec2,6>{{{i,j},{i,j+1},{i+1,j+1},{i,j},{i+1,j+1},{i+1,j}}})
            vertices.push_back({point(ij.x, ij.y), color});
    }
    sphere = upload(vertices, GL_TRIANGLES); vertices.clear();
    for (int i = 0; i < 360; ++i) vertices.push_back({orbit::circle(i*2*orbit::pi/360, 1), glm::vec4(1)});
    ring = upload(vertices, GL_LINE_LOOP); vertices.clear();
    vertices.push_back({{0,0,0}, glm::vec4(1)});
    for (int i = 0; i <= 180; ++i) vertices.push_back({orbit::circle(i*2*orbit::pi/180, 1), glm::vec4(1)});
    disk = upload(vertices, GL_TRIANGLE_FAN); vertices.clear();
    std::mt19937 random(42); std::uniform_real_distribution<float> unit(-1, 1);
    for (int i = 0; i < 1100; ++i) {
        glm::vec3 p(unit(random), unit(random), unit(random));
        if (glm::length(p) < 0.01f) continue;
        float intensity = 0.25f + 0.35f * std::abs(unit(random));
        vertices.push_back({glm::normalize(p)*90.0f, {intensity, intensity, intensity*1.15f, 1}});
    }
    stars = upload(vertices, GL_POINTS);
    overlay = upload({}, GL_TRIANGLES);
}
Renderer::~Renderer() {
    for (auto m : {sphere, ring, disk, stars, overlay}) {
        glDeleteBuffers(1, &m.vbo); glDeleteVertexArrays(1, &m.vao);
    }
    glDeleteProgram(program);
}
void Renderer::draw(const Mesh& mesh, const glm::mat4& transform, glm::vec4 tint) {
    glUseProgram(program);
    glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform4fv(tintLocation, 1, glm::value_ptr(tint));
    glBindVertexArray(mesh.vao); glDrawArrays(mesh.mode, 0, mesh.count);
}
void Renderer::scene(double days, const glm::mat4& vp, bool planes) {
    glEnable(GL_DEPTH_TEST);
    draw(stars, vp, glm::vec4(1));
    draw(ring, vp * model({}, orbit::earthDistance), {0.16f,0.40f,0.56f,1});
    draw(ring, vp * model(orbit::earth(days), orbit::moonDistance, orbit::lunarPlane()), {0.45f,0.76f,0.78f,1});
    draw(sphere, vp * model({}, orbit::sunRadius, glm::rotate(glm::mat4(1),float(orbit::phase(days,25.4)),glm::vec3(0,1,0))), {1,0.63f,0.17f,1});
    draw(sphere, vp * model(orbit::earth(days), orbit::earthRadius, orbit::earthOrientation(days)), {0.16f,0.60f,0.91f,1});
    draw(sphere, vp * model(orbit::moon(days), orbit::moonRadius, orbit::moonOrientation(days)), {0.77f,0.81f,0.87f,1});
    if (planes) {
        glDepthMask(GL_FALSE);
        draw(disk, vp * model(orbit::earth(days), orbit::moonDistance, orbit::lunarPlane()), {0.20f,0.72f,0.78f,0.10f});
        glDepthMask(GL_TRUE);
    }
    glDisable(GL_DEPTH_TEST);
}
void Renderer::overlayDraw(const std::vector<Vertex>& data, int width, int height) {
    glBindBuffer(GL_ARRAY_BUFFER, overlay.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(Vertex), data.data(), GL_STREAM_DRAW);
    overlay.count = static_cast<int>(data.size());
    draw(overlay, glm::ortho(0.0f,float(width),float(height),0.0f), glm::vec4(1));
}
void Renderer::text(float x, float y, float scale, const std::string& value, glm::vec4 color, int width, int height) {
    struct FontVertex { float x, y, z; unsigned char color[4]; };
    std::vector<FontVertex> buffer(value.size()*80 + 4);
    int quads = stb_easy_font_print(0,0,const_cast<char*>(value.c_str()),nullptr,buffer.data(),int(buffer.size()*sizeof(FontVertex)));
    std::vector<Vertex> vertices; vertices.reserve(quads*6);
    for (int q = 0; q < quads; ++q) for (int k : {0,1,2,0,2,3}) {
        auto p = buffer[q*4+k]; vertices.push_back({{x+p.x*scale,y+p.y*scale,0},color});
    }
    overlayDraw(vertices,width,height);
}
void Renderer::panel(float x, float y, float w, float h, glm::vec4 color, int width, int height) {
    overlayDraw({{{x,y,0},color},{{x+w,y,0},color},{{x+w,y+h,0},color},
                 {{x,y,0},color},{{x+w,y+h,0},color},{{x,y+h,0},color}},width,height);
}
void Renderer::screenshot(const std::string& path, int width, int height) {
    if (width <= 0 || height <= 0) throw std::runtime_error("Cannot capture an empty framebuffer");
    std::vector<unsigned char> pixels(size_t(width)*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadBuffer(GL_BACK); glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    stbi_flip_vertically_on_write(1);
    auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    if (!stbi_write_png(path.c_str(),width,height,3,pixels.data(),width*3))
        throw std::runtime_error("Could not write screenshot: " + path);
    std::cout << "Screenshot: " << path << '\n';
}
