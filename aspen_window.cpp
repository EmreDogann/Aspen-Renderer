#include "aspen_window.hpp"
#include <GLFW/glfw3.h>

namespace Aspen {

    AspenWindow::AspenWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
        initWindow();
    }

    AspenWindow::~AspenWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void AspenWindow::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Prevent GLFW from creating an OpenGL context.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // Prevent GLFW from making the window resizeable.

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    }
}