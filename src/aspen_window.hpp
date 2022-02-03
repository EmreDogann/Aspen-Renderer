#pragma once
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <functional>
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

        bool wasWindowResized() { return framebufferResized; };
        void resetWindowResizedFlag() { framebufferResized = false; };

        void windowResizeSubscribe(std::function<void()> subscriber) { windowResizeSubscribers.push_back(subscriber); };
        void notifySubscribers();

    private:
        static void framebufferResizedCallback(GLFWwindow *window, int width, int height);
        void initWindow();

        int width;
        int height;
        bool framebufferResized = false;

        std::string windowName;
        GLFWwindow *window;

        std::vector<std::function<void()>> windowResizeSubscribers;
    };

} // namespace Aspen