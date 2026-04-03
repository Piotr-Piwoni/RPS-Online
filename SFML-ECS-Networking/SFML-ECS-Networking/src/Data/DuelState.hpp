#pragma once
#include <cstdint>

#include "../Networking/RPSChoice.hpp"

namespace Game
{
struct DuelState
{
	bool InDuel{false};
	std::uint32_t OpponentID{0};
	RPSChoice Choice{RPSChoice::NONE};
};
}
