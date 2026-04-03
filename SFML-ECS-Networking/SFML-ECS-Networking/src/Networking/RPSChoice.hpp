#pragma once
#include <cstdint>

namespace Game
{
enum class RPSChoice : std::uint8_t
{
	NONE,
	ROCK,
	PAPER,
	SCISSORS
};

inline const char* ToString(const RPSChoice e)
{
	switch (e)
	{
	case RPSChoice::NONE: return "NONE";
	case RPSChoice::ROCK: return "ROCK";
	case RPSChoice::PAPER: return "PAPER";
	case RPSChoice::SCISSORS: return "SCISSORS";
	default: return "unknown";
	}
}
}
