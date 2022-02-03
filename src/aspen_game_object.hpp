#pragma once

#include "aspen_model.hpp"

// Std
#include <memory>

namespace Aspen {
    struct Transform2dComponent {
        glm::vec2 translation{}; // Position offset.
        glm::vec2 scale{1.0f, 1.0f};
        float rotation;

        glm::mat2 mat2() {
            const float s = glm::sin(rotation);
            const float c = glm::cos(rotation);
            glm::mat2 rotMatrix{{c, s}, {-s, c}};

            glm::mat2 scaleMat{{scale.x, 0.0f}, {0.0f, scale.y}}; // NOTE: The GLM mat constructor takes each argument as a COLUMN and not a row.
            return rotMatrix * scaleMat;
        }
    };

    class AspenGameObject {
    public:
        using id_t = unsigned int;

        static AspenGameObject createGameObject() {
            static id_t currentId = 0;
            return AspenGameObject{currentId++};
        }

        AspenGameObject(const AspenGameObject &) = delete;
        AspenGameObject &operator=(const AspenGameObject &) = delete;
        AspenGameObject(AspenGameObject &&) = default;
        AspenGameObject &operator=(AspenGameObject &&) = default;

        const id_t getId() { return id; };

        std::shared_ptr<AspenModel> model{};
        glm::vec3 color{};
        Transform2dComponent transform2d{};

    private:
        AspenGameObject(id_t objId) : id{objId} {}
        id_t id;
    };
}