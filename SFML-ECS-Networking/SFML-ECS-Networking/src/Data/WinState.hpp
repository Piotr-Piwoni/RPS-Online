#pragma once
#include <cstdint>

namespace Game
{
enum class WinState : std::uint8_t
{
	NONE,
	WON,
	LOST
};

inline const char* ToString(const WinState e)
{
	switch (e)
	{
	case WinState::NONE: return "NONE";
	case WinState::WON: return "WON";
	case WinState::LOST: return "LOST";
	default: return "unknown";
	}
}
}
