#include "orbits.hpp"
#include <iostream>
#include <stdexcept>
void require(bool pass, const char* message) {
    if (!pass) throw std::runtime_error(message);
}
int main() {
    try {
        for (double d : {-10000.0,0.0,3.0,48.0,100.0,1000000.0}) {
            require(std::abs(glm::length(orbit::earth(d))-orbit::earthDistance)<1e-4f,"Earth orbital radius");
            auto offset=orbit::moon(d)-orbit::earth(d);
            require(std::abs(glm::length(offset)-orbit::moonDistance)<1e-4f,"Moon must follow Earth at constant distance");
            auto normal=glm::vec3(orbit::lunarPlane()*glm::vec4(0,1,0,0));
            require(std::abs(glm::dot(offset,normal))<1e-4f,"Moon lies in its tilted plane");
            auto facing=glm::vec3(orbit::moonOrientation(d)*glm::vec4(-1,0,0,0));
            require(glm::dot(facing,-glm::normalize(offset))>0.9999f,"Moon tidally locked to Earth");
            require(glm::length(orbit::earth(d)-orbit::earth(d+orbit::earthPeriod))<1e-4f,"Earth periodicity");
            require(glm::length(orbit::moonOffset(d)-orbit::moonOffset(d+orbit::moonPeriod))<1e-4f,"Moon periodicity");
        }
        require(std::abs(orbit::moonOffset(orbit::moonPeriod/4).y)>0.2f,"Orbits must not be coplanar");
        require(orbit::earthPeriod/orbit::moonPeriod>13 && orbit::earthPeriod/orbit::moonPeriod<14,"Relative orbital speed");
        auto axis=glm::vec3(orbit::earthOrientation(2)*glm::vec4(0,1,0,0));
        require(std::abs(glm::dot(axis,glm::vec3(0,1,0))-std::cos(glm::radians(23.44f)))<1e-5f,"Earth obliquity");
        std::cout<<"Orbital invariants passed\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
