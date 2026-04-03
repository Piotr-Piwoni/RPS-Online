#pragma once
#include <cstdint>
#include <SFML/Network/Packet.hpp>

namespace Game
{
enum class PacketType : std::uint8_t
{
	JOIN,
	UPDATE_MOVEMENT,
	UPDATE_CLIENTS,
	MESSAGE,
	CLIENT_DISCONNECT,
	NEW_HOST,
	DUEL_REQUEST,
	DUEL_CHOICE,
	DUEL_START,
	DUEL_RESULT,
	PING,
	PONG
};

inline const char* ToString(const PacketType e)
{
	switch (e)
	{
	case PacketType::JOIN: return "JOIN";
	case PacketType::UPDATE_MOVEMENT: return "UPDATE_MOVEMENT";
	case PacketType::UPDATE_CLIENTS: return "UPDATE_CLIENTS";
	case PacketType::MESSAGE: return "MESSAGE";
	case PacketType::CLIENT_DISCONNECT: return "CLIENT_DISCONNECT";
	case PacketType::NEW_HOST: return "NEW_HOST";
	case PacketType::DUEL_REQUEST: return "DUEL_REQUEST";
	case PacketType::DUEL_CHOICE: return "DUEL_CHOICE";
	case PacketType::DUEL_START: return "DUEL_START";
	case PacketType::DUEL_RESULT: return "DUEL_RESULT";
	case PacketType::PING: return "PING";
	case PacketType::PONG: return "PONG";
	default: return "unknown";
	}
}
}
