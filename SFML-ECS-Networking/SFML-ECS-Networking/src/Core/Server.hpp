#pragma once
#include <optional>
#include <random>
#include <vector>
#include <SFML/Network.hpp>
#include <SFML/Graphics/Color.hpp>

#include "../Data/PlayerData.hpp"
#include "../Data/WinState.hpp"
#include "../Networking/Packet.hpp"
#include "../Utilities/Aliases.hpp"
#include "../Utilities/Log.hpp"

namespace Game
{
class Server
{
	struct HostData
	{
		std::uint32_t ID{0};
		sf::IpAddress Address{0};
		Port Port{0};
	};

public:
	Server();

	bool Start(Port port);
	void Update(float dt);
	void Stop();

	bool IsRunning() const;

	std::uint32_t GetHostID() const;
	void UpdateHostID(std::uint32_t id);

	static sf::IpAddress GetServerAddress();
	Port GetServerPort() const;

private:
	void AcceptNewClient();
	void BroadcastStates();
	void DisconnectPlayer(PlayerData& player);
	std::optional<HostData> ElectNewHost() const;
	static WinState ResolveRPS(RPSChoice a, RPSChoice b);
	bool HandleDuelStart(std::vector<PlayerData>::value_type& playerData,
						 std::uint32_t targetID);
	bool HandleDuel(std::vector<PlayerData>::value_type& playerData);

private:
	std::vector<PlayerData> m_Players{};
	HostData m_Host{};

	sf::TcpListener m_Listener{};
	sf::SocketSelector m_Selector{};
	bool m_IsRunning{false};

	std::mt19937 m_Generator{};
	float m_ServerTime{0.f};
};
}
