#pragma once
#include "aspen_device.hpp"
#include "aspen_game_object.hpp"
#include "aspen_model.hpp"
#include "aspen_renderer.hpp"
#include "aspen_window.hpp"

// std
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Aspen {
    class FirstApp {
    public:
        static constexpr int WIDTH = 1024;
        static constexpr int HEIGHT = 768;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp &) = delete;
        FirstApp &operator=(const FirstApp &);
        void run();

    private:
        void loadGameObjects();

        AspenWindow aspenWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
        AspenDevice aspenDevice{aspenWindow};
        AspenRenderer aspenRenderer{aspenWindow, aspenDevice};

        std::vector<AspenGameObject> gameObjects;
    };
}