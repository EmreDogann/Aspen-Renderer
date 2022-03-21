#include "Aspen/Core/uuid.hpp"

// Below is an example of how to use a hashing function to use the UUIDs as a key in an unordered map.
// static std::unordered_map<Aspen::UUID, std::string> m_Map;
// static void AddToMap() {
//     m_Map[Aspen::UUID()] = "Aspen";
// }

namespace Aspen {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	UUID::UUID()
	    : m_UUID(s_UniformDistribution(s_Engine)) {
	}

	UUID::UUID(uint64_t uuid)
	    : m_UUID(uuid) {
	}
} // namespace Aspen