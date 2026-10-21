#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace orbit {
constexpr double pi = 3.14159265358979323846;
constexpr double earthPeriod = 365.256;
constexpr double moonPeriod = 27.322;
constexpr double earthSpinPeriod = 0.99727;
constexpr float earthDistance = 10.0f;
constexpr float moonDistance = 2.25f;
constexpr float inclination = 5.145f;
constexpr float sunRadius = 1.65f;
constexpr float earthRadius = 0.63f;
constexpr float moonRadius = 0.23f;

inline double phase(double days, double period) {
    return 2.0 * pi * std::remainder(days, period) / period;
}
// Y is up. Both revolutions are prograde when seen from +Y.
inline glm::vec3 circle(double angle, float radius) {
    return {radius * float(std::cos(angle)), 0, -radius * float(std::sin(angle))};
}
inline glm::mat4 lunarPlane() {
    return glm::rotate(glm::mat4(1), glm::radians(inclination), glm::vec3(1, 0, 0));
}
inline glm::vec3 earth(double days) { return circle(phase(days, earthPeriod), earthDistance); }
inline glm::vec3 moonOffset(double days) {
    return glm::vec3(lunarPlane() * glm::vec4(circle(phase(days, moonPeriod), moonDistance), 0));
}
inline glm::vec3 moon(double days) { return earth(days) + moonOffset(days); }
inline glm::mat4 earthOrientation(double days) {
    return glm::rotate(glm::rotate(glm::mat4(1), glm::radians(23.44f), glm::vec3(0, 0, 1)),
                       float(phase(days, earthSpinPeriod)), glm::vec3(0, 1, 0));
}
// Local -X always faces Earth: synchronous rotation, not an independent moon spin.
inline glm::mat4 moonOrientation(double days) {
    return glm::rotate(lunarPlane(), float(phase(days, moonPeriod)), glm::vec3(0, 1, 0));
}
}
