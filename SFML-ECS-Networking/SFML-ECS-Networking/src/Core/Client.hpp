#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include "../Data/PlayerData.hpp"
#include "../Data/WinState.hpp"
#include "../Networking/Packet.hpp"
#include "../Utilities/Aliases.hpp"
#include "../Utilities/Log.hpp"


namespace Game
{
class Client
{
public:
	void Connect(const sf::IpAddress& addr, Port port);
	void SendInput(sf::Vector2f input);
	void UpdateState();
	void Render(sf::RenderWindow& window) const;

	bool WantReconnection() const;
	bool IsConnected() const;
	void Disconnect();

	bool NewHostElected() const;
	bool BecomeHost() const;
	void ClearHostFlag();

	bool InDuel() const;
	bool HasChosen() const;
	void RequestDuel(std::uint32_t targetID);
	void SendRPS(RPSChoice choice);
	std::optional<std::uint32_t> FindNearestDuelTarget(float maxRange) const;

	void SendPing();

	std::uint32_t GetID() const;
	sf::IpAddress GetHostAddress() const;
	Port GetHostPort() const;
	sf::Vector2f GetPosition() const;
	sf::Vector2f GetSize() const;
	WinState& GetWinState();
	void PrintLatency() const;

private:
	void InterpolatePlayers();

private:
	sf::TcpSocket m_Socket{};
	std::uint32_t m_ID{};
	bool m_Connected{false};

	sf::IpAddress m_HostIP{0};
	Port m_HostPort{0};

	std::vector<PlayerData> m_Players{};
	std::size_t m_DataIndex{SIZE_MAX};

	bool m_BecomeHost{false};
	bool m_NewHostElected{false};
	bool m_Reconnect{false};
	WinState m_WinState{WinState::NONE};

	float m_ServerTime{0.f};
	float m_LastPingSendTime{0.f};
	float m_RTT{0.f};
};
}
