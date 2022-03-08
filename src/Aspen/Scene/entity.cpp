#include "entity.hpp"

namespace Aspen {
	Entity::Entity(entt::entity handle, Scene* scene) : m_EntityHandle(handle), m_Scene(scene) {}
} // namespace Aspen