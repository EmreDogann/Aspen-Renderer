#pragma once
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace Aspen {

    class AspenWindow {
    public:
        AspenWindow(int x, int h, std::string name);
        ~AspenWindow();

        // Remove copy constructor and operator to prevent accidental copy creation of window, possibly leading to a dangling pointer.
        AspenWindow(const AspenWindow &) = delete;
        AspenWindow &operator=(const AspenWindow &);

        bool shouldClose() { return glfwWindowShouldClose(window); }
        VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    private:
        void initWindow();

        const int width;
        const int height;

        std::string windowName;
        GLFWwindow *window;
    };

} // namespace Aspen