#include "first_app.hpp"
#include <GLFW/glfw3.h>

namespace Aspen {
    void FirstApp::run() {

        while (!aspenWindow.shouldClose()) {
            glfwPollEvents(); // Collect window level events.
        }
    }
}