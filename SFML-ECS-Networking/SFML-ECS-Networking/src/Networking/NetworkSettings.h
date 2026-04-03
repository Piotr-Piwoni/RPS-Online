#pragma once
#include <cstdint>

#include "../Utilities/Aliases.hpp"

namespace Game
{
struct NetworkSettings
{
	NetworkSettings() = default;
	NetworkSettings(const NetworkSettings&) = delete;

	NetworkSettings& operator=(const NetworkSettings&) = delete;

	static NetworkSettings& Get()
	{
		static NetworkSettings instance;
		return instance;
	}

	Port Port{54000};
	std::uint32_t MaxPlayers{3};
	float InterpDelay{0.1f};
};
}
