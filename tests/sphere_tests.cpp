#include "sphere.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

void require(bool pass, const char* message) {
    if (!pass) throw std::runtime_error(message);
}
int main() {
    try {
        constexpr int slices = 96, stacks = 48;
        auto mesh = makeSphere(slices, stacks);
        require(mesh.size() == slices * stacks * 6, "Triangle count");
        for (const auto& v : mesh) {
            require(std::abs(glm::length(v.position) - 1) < 1e-5f, "Unit sphere");
            require(glm::length(v.normal-v.position) < 1e-5f, "Outward unit normals");
            require(v.uv.x >= 0 && v.uv.x <= 1 && v.uv.y >= 0 && v.uv.y <= 1, "UV range");
            if (v.uv.y == 0) require(v.position.y > 0.9999f, "JPEG north at V=0");
            if (v.uv.y == 1) require(v.position.y < -0.9999f, "JPEG south at V=1");
        }
        for (size_t i = 0; i < mesh.size(); i += 3) {
            auto a=mesh[i], b=mesh[i+1], c=mesh[i+2];
            float span=std::max({a.uv.x,b.uv.x,c.uv.x})-std::min({a.uv.x,b.uv.x,c.uv.x});
            require(span < 1.01f/slices, "No interpolation across longitude seam");
            auto cross=glm::cross(b.position-a.position,c.position-a.position);
            if (glm::length(cross)>1e-6f)
                require(glm::dot(cross,a.normal)>0, "Outward winding");
        }
        for (int row=0; row<stacks; ++row) {
            auto a=mesh[row*slices*6];
            auto b=mesh[(row*slices+slices-1)*6+5];
            require(a.uv.x==0 && b.uv.x==1, "Seam UV endpoints duplicated");
            require(glm::length(a.position-b.position)<1e-5f, "Seam geometry continuous");
        }
        // UV increases to the viewer's right when looking toward local +X.
        auto equator = mesh[(stacks/2)*slices*6];
        auto east = mesh[(stacks/2)*slices*6+5];
        require(east.uv.x>equator.uv.x && east.position.z<equator.position.z, "Map not mirrored");
        std::cout << "Sphere normals, UV orientation and seam invariants passed\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
