#pragma once
#include "aspen_window.hpp"

namespace Aspen {
    class FirstApp {
    public:
        static constexpr int WIDTH = 1024;
        static constexpr int HEIGHT = 768;

        void run();

    private:
        AspenWindow aspenWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    };
}