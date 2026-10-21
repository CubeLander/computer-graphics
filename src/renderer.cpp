#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define STB_EASY_FONT_IMPLEMENTATION
#include <stb_easy_font.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include <stb_image.h>
#include "texture_data.hpp"
#include "renderer.hpp"
#include "orbits.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <memory>
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
unsigned loadTexture(const unsigned char* bytes, int size, const char* name) {
    int width = 0, height = 0, channels = 0;
    std::unique_ptr<unsigned char, decltype(&stbi_image_free)> pixels(
        stbi_load_from_memory(bytes, size, &width, &height, &channels, 3), stbi_image_free);
    if (!pixels) throw std::runtime_error(std::string("Texture decode failed: ") + name);
    unsigned texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Hardware decodes sRGB to linear BEFORE filtering and lighting.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &texture);
        throw std::runtime_error(std::string("Texture upload failed: ") + name);
    }
    std::cout << "Texture: " << name << " " << width << "x" << height << '\n';
    return texture;
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
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    return m;
}
Renderer::Renderer() {
    try {
        const char* vs = R"(#version 330 core
    layout(location=0) in vec3 position;
    layout(location=1) in vec4 color;
    layout(location=2) in vec3 normal;
    layout(location=3) in vec2 uv;
    uniform mat4 transform;
    uniform mat4 model;
    uniform mat3 normalMatrix;
    uniform vec4 tint;
    out vec4 vertexColor;
    out vec3 worldPosition;
    out vec3 worldNormal;
    out vec2 texCoord;
    void main() {
        gl_Position = transform * vec4(position, 1);
        vertexColor = color * tint;
        worldPosition = vec3(model * vec4(position, 1));
        worldNormal = normalMatrix * normal;
        texCoord = uv;
    }
    )";
        const char* fs = R"(#version 330 core
    in vec4 vertexColor;
    in vec3 worldPosition;
    in vec3 worldNormal;
    in vec2 texCoord;
    uniform sampler2D albedoMap;
    uniform vec3 eyePosition;
    uniform int surface; // 0: assignment one / HUD, 1: lit body, 2: emissive Sun
    uniform float specularStrength;
    out vec4 fragColor;
    vec3 toSRGB(vec3 linear) {
        linear = max(linear, vec3(0));
        return mix(12.92 * linear, 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055,
                   step(vec3(0.0031308), linear));
    }
    void main() {
        if (surface == 0) { fragColor = vertexColor; return; }
        vec3 albedo = texture(albedoMap, texCoord).rgb;
        if (surface == 2) { fragColor = vec4(toSRGB(albedo), 1); return; }
        vec3 N = normalize(worldNormal);
        // The point source is the Sun's center at the world origin, not the camera.
        vec3 L = normalize(-worldPosition);
        vec3 V = normalize(eyePosition - worldPosition);
        float diffuse = max(dot(N, L), 0.0);
        vec3 halfway = L + V;
        float specular = 0.0;
        if (diffuse > 0.0 && dot(halfway, halfway) > 0.000001)
            specular = specularStrength * pow(max(dot(N, normalize(halfway)), 0.0), 48.0);
        // A very small pedagogical ambient term reveals the night-side silhouette.
        // No distance falloff: scene distances are deliberately not astronomical.
        vec3 linear = albedo * (0.018 + 0.95 * diffuse) + vec3(specular);
        fragColor = vec4(toSRGB(linear), 1);
    }
    )";
        unsigned v = compile(GL_VERTEX_SHADER, vs), f = 0;
        try { f = compile(GL_FRAGMENT_SHADER, fs); }
        catch (...) { glDeleteShader(v); throw; }
        program = glCreateProgram(); glAttachShader(program, v); glAttachShader(program, f);
        glLinkProgram(program); glDeleteShader(v); glDeleteShader(f);
        int ok; glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[4096]; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            throw std::runtime_error(std::string("Shader link: ") + log);
        }
        transformLocation = glGetUniformLocation(program, "transform");
        tintLocation = glGetUniformLocation(program, "tint");
        modelLocation = glGetUniformLocation(program, "model");
        normalLocation = glGetUniformLocation(program, "normalMatrix");
        eyeLocation = glGetUniformLocation(program, "eyePosition");
        surfaceLocation = glGetUniformLocation(program, "surface");
        specularLocation = glGetUniformLocation(program, "specularStrength");
        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program, "albedoMap"), 0);
        earthTexture = loadTexture(textures::earth, static_cast<int>(sizeof(textures::earth)), "Earth");
        moonTexture = loadTexture(textures::moon, static_cast<int>(sizeof(textures::moon)), "Moon");
        sunTexture = loadTexture(textures::sun, static_cast<int>(sizeof(textures::sun)), "Sun");
        sphere = upload(makeSphere(96, 48), GL_TRIANGLES);
        std::vector<Vertex> vertices;
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
    } catch (...) { release(); throw; }
}
Renderer::~Renderer() { release(); }
void Renderer::release() {
    for (auto m : {sphere, ring, disk, stars, overlay}) {
        glDeleteBuffers(1, &m.vbo); glDeleteVertexArrays(1, &m.vao);
    }
    glDeleteProgram(program);
    for (auto texture : {earthTexture, moonTexture, sunTexture}) glDeleteTextures(1, &texture);
}
void Renderer::draw(const Mesh& mesh, const glm::mat4& transform, glm::vec4 tint,
                    const glm::mat4& modelMatrix, int surface, unsigned texture, float specular) {
    glUseProgram(program);
    glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform4fv(tintLocation, 1, glm::value_ptr(tint));
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    glUniformMatrix3fv(normalLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform1i(surfaceLocation, surface);
    glUniform1f(specularLocation, specular);
    // Keep a complete sampler bound even for unlit branches (macOS driver validation).
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texture ? texture : earthTexture);
    glBindVertexArray(mesh.vao); glDrawArrays(mesh.mode, 0, mesh.count);
}
void Renderer::scene(double days, const glm::mat4& vp, glm::vec3 eye, bool planes, bool textured) {
    glUseProgram(program);
    glUniform3fv(eyeLocation, 1, glm::value_ptr(eye));
    glEnable(GL_DEPTH_TEST);
    draw(stars, vp, glm::vec4(1));
    draw(ring, vp * model({}, orbit::earthDistance), {0.16f,0.40f,0.56f,1});
    draw(ring, vp * model(orbit::earth(days), orbit::moonDistance, orbit::lunarPlane()), {0.45f,0.76f,0.78f,1});
    // Educational plane is composited behind opaque bodies, never tinting their surfaces.
    if (planes) {
        glDepthMask(GL_FALSE);
        draw(disk, vp * model(orbit::earth(days), orbit::moonDistance, orbit::lunarPlane()), {0.20f,0.72f,0.78f,0.10f});
        glDepthMask(GL_TRUE);
    }
    auto sunModel = model({}, orbit::sunRadius,
        glm::rotate(glm::mat4(1), float(orbit::phase(days,25.4)), glm::vec3(0,1,0)));
    auto earthModel = model(orbit::earth(days), orbit::earthRadius, orbit::earthOrientation(days));
    auto moonModel = model(orbit::moon(days), orbit::moonRadius, orbit::moonOrientation(days));
    draw(sphere, vp * sunModel, {1,0.63f,0.17f,1}, sunModel, textured ? 2 : 0, sunTexture);
    draw(sphere, vp * earthModel, {0.16f,0.60f,0.91f,1}, earthModel, textured ? 1 : 0, earthTexture, 0.08f);
    draw(sphere, vp * moonModel, {0.77f,0.81f,0.87f,1}, moonModel, textured ? 1 : 0, moonTexture, 0.008f);
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
