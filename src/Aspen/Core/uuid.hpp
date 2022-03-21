#pragma once

#include "pch.h"

// Ideally the UUID would be a 128 bit number as that is traditionally how they are defined, but for the purposes of a game engine this is fine.
// Adapted from: https://www.youtube.com/watch?v=O_0nUE4S8T8
namespace Aspen {
	class UUID {
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const {
			return m_UUID;
		}

	private:
		uint64_t m_UUID;
	};
} // namespace Aspen

namespace std {
	template <>
	struct hash<Aspen::UUID> {
		std::size_t operator()(const Aspen::UUID& uuid) const {
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
} // namespace std