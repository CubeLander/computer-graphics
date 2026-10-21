#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "renderer.hpp"
#include "orbits.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
struct App {
    double days = 48, speed = 5;
    float yaw = 0.65f, elevation = 0.65f, distance = 29;
    bool paused = false, planes = true, dragging = false;
    bool capture = false;
    double lastX = 0, lastY = 0;
    int view = 1;
};
App& state(GLFWwindow* window) { return *static_cast<App*>(glfwGetWindowUserPointer(window)); }
void preset(App& a, int view) {
    a.view = view;
    a.yaw = 0.65f;
    a.elevation = view == 3 ? 1.55f : 0.65f;
    a.distance = view == 2 ? 7.5f : 29;
}
void key(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto& a = state(w);
    switch (key) {
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, GLFW_TRUE); break;
    case GLFW_KEY_SPACE: a.paused = !a.paused; break;
    case GLFW_KEY_P: a.planes = !a.planes; break;
    case GLFW_KEY_R: a.days = 48; a.speed = 5; preset(a,1); break;
    case GLFW_KEY_EQUAL: case GLFW_KEY_KP_ADD: a.speed = std::min(80.0, a.speed*2); break;
    case GLFW_KEY_MINUS: case GLFW_KEY_KP_SUBTRACT: a.speed = std::max(0.078125, a.speed/2); break;
    case GLFW_KEY_1: preset(a,1); break;
    case GLFW_KEY_2: preset(a,2); break;
    case GLFW_KEY_3: preset(a,3); break;
    case GLFW_KEY_F12: a.capture = true; break;
    }
}
struct Options {
    int frames = 0, view = 1;
    double days = 48;
    bool fixed = false;
    std::string screenshot;
};
Options options(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            std::cout << "Sun Earth Moon - OpenGL 3.3 core\n"
                "Options: --days NUMBER (fixed simulation time), --view 1|2|3,\n"
                "         --frames POSITIVE_INTEGER (exit after N frames), --screenshot FILE.png\n"
                "Controls: drag to orbit; wheel to zoom; Space pause; +/- speed;\n"
                "          1 overview; 2 follow Earth; 3 top; P orbital plane; R reset; F12 capture; Esc quit.\n";
            std::exit(0);
        }
        if (i+1 >= argc) throw std::runtime_error("Missing value for " + arg);
        std::string value = argv[++i]; size_t consumed = 0;
        if (arg == "--screenshot") { o.screenshot = value; continue; }
        if (arg == "--days") {
            o.days = std::stod(value,&consumed); o.fixed = true;
            if (!std::isfinite(o.days)) throw std::runtime_error("Days must be finite");
        } else if (arg == "--view") {
            o.view = std::stoi(value,&consumed);
            if (o.view < 1 || o.view > 3) throw std::runtime_error("View must be 1, 2 or 3");
        } else if (arg == "--frames") {
            o.frames = std::stoi(value,&consumed);
            if (o.frames <= 0) throw std::runtime_error("Frame count must be positive");
        } else throw std::runtime_error("Unknown option: " + arg);
        if (consumed != value.size()) throw std::runtime_error("Invalid value for " + arg);
    }
    if (!o.screenshot.empty() && o.frames == 0) o.frames = 3;
    return o;
}
void label(Renderer& r, const glm::mat4& vp, glm::vec3 pos, const std::string& text,
           glm::vec4 color, int w, int h) {
    glm::vec4 clip = vp * glm::vec4(pos,1);
    if (clip.w <= 0) return;
    glm::vec3 ndc = glm::vec3(clip)/clip.w;
    if (std::abs(ndc.x)>1 || std::abs(ndc.y)>1 || std::abs(ndc.z)>1) return;
    r.text((ndc.x*0.5f+0.5f)*w+12, (0.5f-ndc.y*0.5f)*h-12, 1.4f, text, color, w,h);
}
void run(GLFWwindow* window, const Options& o) {
    App a; a.days = o.days; a.paused = o.fixed; preset(a,o.view);
    glfwSetWindowUserPointer(window,&a);
    glfwSetKeyCallback(window,key);
    glfwSetMouseButtonCallback(window,[](GLFWwindow* w,int button,int action,int) {
        if (button != GLFW_MOUSE_BUTTON_LEFT) return;
        auto& a = state(w); a.dragging = action == GLFW_PRESS;
        glfwGetCursorPos(w,&a.lastX,&a.lastY);
    });
    glfwSetCursorPosCallback(window,[](GLFWwindow* w,double x,double y) {
        auto& a = state(w);
        if (a.dragging) {
            a.yaw -= float(x-a.lastX)*0.006f;
            a.elevation = std::clamp(a.elevation+float(y-a.lastY)*0.006f,-1.55f,1.55f);
        }
        a.lastX=x; a.lastY=y;
    });
    glfwSetScrollCallback(window,[](GLFWwindow* w,double,double y) {
        auto& a = state(w);
        a.distance = std::clamp(a.distance*std::exp(-float(y)*0.1f),a.view==2 ? 3.5f : 16.0f,65.0f);
    });
    Renderer renderer;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    double previous = glfwGetTime(); int frames = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now = glfwGetTime(), dt = std::min(now-previous,0.1); previous=now;
        if (!a.paused) a.days += dt*a.speed;
        int fw,fh,w,h; glfwGetFramebufferSize(window,&fw,&fh); glfwGetWindowSize(window,&w,&h);
        if (fw == 0 || fh == 0 || w == 0 || h == 0) { glfwWaitEventsTimeout(0.1); continue; }
        glViewport(0,0,fw,fh); glClearColor(0.018f,0.028f,0.055f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glm::vec3 target = a.view == 2 ? orbit::earth(a.days) : glm::vec3(0);
        glm::vec3 eye = target + a.distance*glm::vec3(std::cos(a.elevation)*std::sin(a.yaw),
            std::sin(a.elevation), std::cos(a.elevation)*std::cos(a.yaw));
        auto vp = glm::perspective(glm::radians(45.0f),float(fw)/fh,0.1f,180.0f) *
                  glm::lookAt(eye,target,glm::vec3(0,1,0));
        renderer.scene(a.days,vp,a.planes);
        label(renderer,vp,{0,orbit::sunRadius+0.2f,0},"SUN",{1,0.75f,0.35f,1},w,h);
        label(renderer,vp,orbit::earth(a.days)+glm::vec3(0,0.85f,0),"EARTH",{0.4f,0.8f,1,1},w,h);
        label(renderer,vp,orbit::moon(a.days)+glm::vec3(0,0.35f,0),"MOON",{0.83f,0.89f,0.95f,1},w,h);
        renderer.panel(20,20,435,145,{0.025f,0.045f,0.08f,0.94f},w,h);
        renderer.panel(20,20,3,145,{0.3f,0.8f,0.88f,1},w,h);
        renderer.text(38,36,2.5f,"SUN / EARTH / MOON",{0.9f,0.95f,1,1},w,h);
        renderer.text(38,67,1.4f,"01  /  ORBITAL MOTION",{0.35f,0.8f,0.88f,1},w,h);
        std::ostringstream status;
        status << std::fixed << std::setprecision(2) << "DAY " << a.days << "   |   "
               << (a.paused ? "PAUSED" : "RUNNING") << "   |   " << a.speed << " d/s";
        renderer.text(38,92,1.25f,status.str(),{0.8f,0.85f,0.9f,1},w,h);
        renderer.text(38,116,1.2f,"YEAR 365.256 d   /   LUNAR ORBIT 27.322 d",{0.6f,0.7f,0.8f,1},w,h);
        renderer.text(38,138,1.2f,"LUNAR PLANE +5.145 deg   /   SIZES NOT TO SCALE",{0.6f,0.7f,0.8f,1},w,h);
        renderer.panel(20,float(h)-70,float(w)-40,50,{0.025f,0.045f,0.08f,0.94f},w,h);
        renderer.text(36,float(h)-57,1.25f,"DRAG orbit   SCROLL zoom   SPACE pause   +/- speed   1/2/3 views",{0.75f,0.84f,0.92f,1},w,h);
        renderer.text(36,float(h)-36,1.1f,"P lunar plane   R reset   F12 screenshot   ESC quit   |   OpenGL 3.3 Core",{0.45f,0.6f,0.72f,1},w,h);
        ++frames;
        if (a.capture || (!o.screenshot.empty() && frames == o.frames)) {
            renderer.screenshot(a.capture ? "screenshot.png" : o.screenshot,fw,fh); a.capture=false;
        }
        unsigned error = glGetError();
        if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
        glfwSwapBuffers(window);
        if (o.frames && frames >= o.frames) break;
    }
}
}
int main(int argc,char** argv) {
    GLFWwindow* window = nullptr;
    try {
        auto o = options(argc,argv);
        glfwSetErrorCallback([](int code,const char* message) { std::cerr<<"GLFW "<<code<<": "<<message<<'\n'; });
        if (!glfwInit()) throw std::runtime_error("GLFW initialization failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES,4);
        window = glfwCreateWindow(1280,800,"Sun / Earth / Moon - Assignment 01",nullptr,nullptr);
        if (!window) throw std::runtime_error("Cannot create an OpenGL 3.3 Core window");
        glfwSetWindowSizeLimits(window,900,560,GLFW_DONT_CARE,GLFW_DONT_CARE);
        glfwMakeContextCurrent(window); glfwSwapInterval(1);
        if (!gladLoadGL(glfwGetProcAddress) || !GLAD_GL_VERSION_3_3)
            throw std::runtime_error("OpenGL 3.3 or newer is required");
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\nGPU: " << glGetString(GL_RENDERER) << '\n';
        run(window,o);
        glfwDestroyWindow(window); glfwTerminate();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        if (window) glfwDestroyWindow(window);
        glfwTerminate(); return 1;
    }
}
