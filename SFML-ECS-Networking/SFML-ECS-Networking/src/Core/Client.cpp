#include "Client.hpp"

#include "../Networking/NetworkSettings.h"

void Game::Client::Connect(const sf::IpAddress& addr, const Port port)
{
	if (m_Socket.connect(addr, port) != sf::Socket::Status::Done)
	{
		Log::PrintMsg("Failed to connect to server.", ERROR);
		return;
	}

	// Update host related variables.
	m_NewHostElected = false;
	m_Reconnect = false;
	m_HostIP = addr;
	m_HostPort = port;

	m_Socket.setBlocking(false);
	m_Connected = true;
	Log::PrintMsg("Connected to server.", SUCCESS);
}

void Game::Client::SendInput(const sf::Vector2f input)
{
	// Return if not connected, or
	// we haven't received the data tracking index yet.
	if (!m_Connected || m_DataIndex >= m_Players.size())
		return;

	sf::Packet packet;
	packet << static_cast<std::uint8_t>(PacketType::UPDATE_MOVEMENT);
	packet << input.x << input.y;

	if (m_Socket.send(packet) != sf::Socket::Status::Done)
	{
		const auto msg = Log::Format(
				"Had a problem sending over the {} packet to server!",
				ToString(PacketType::UPDATE_MOVEMENT));
		Log::PrintMsg(msg, ERROR);
	}
}

void Game::Client::UpdateState()
{
	if (!m_Connected)
		return;

	while (m_Connected)
	{
		sf::Packet packet;
		const auto status = m_Socket.receive(packet);

		if (status != sf::Socket::Status::Done)
		{
			if (status == sf::Socket::Status::NotReady)
				break; //< No more data.

			// Otherwise, lost connection.
			m_Connected = false;
			break;
		}

		// Read packet type header.
		std::uint8_t typeRaw{};
		if (!(packet >> typeRaw))
			continue; //< Corrupt packet.

		switch (static_cast<PacketType>(typeRaw))
		{
		case PacketType::UPDATE_MOVEMENT: break; //< Server only packet type.
		case PacketType::MESSAGE:
			{
				std::string msg;
				std::uint8_t type;
				packet >> msg >> type;
				Log::PrintMsg("Server says: " + msg,
							  static_cast<MessageType>(type));
				break;
			}
		case PacketType::UPDATE_CLIENTS:
			{
				// Update the server time.
				packet >> m_ServerTime;

				std::uint32_t count{};
				packet >> count;

				// See if the number of players needs to be updated.
				if (m_Players.size() != count)
					m_Players.resize(count);

				// Update player data.
				for (std::size_t i = 0; i < count; i++)
				{
					auto& player = m_Players[i];

					std::uint32_t colourInt{};
					std::uint8_t active{};
					std::uint8_t inDuel{};
					std::uint8_t rpsRawType{};
					packet >> player.ID
							>> player.Position.x >> player.Position.y
							>> colourInt
							>> active
							>> inDuel
							>> player.Duel.OpponentID
							>> rpsRawType;

					player.Colour = sf::Color(colourInt);
					player.Connected = active == 1;
					player.Duel.InDuel = inDuel == 1;
					player.Duel.Choice = static_cast<RPSChoice>(rpsRawType);

					// Add the player position to the snapshot.
					player.Snapshots.push_back({
						.Position = player.Position,
						.TimeStamp = m_ServerTime
					});

					// Remove old snapshots.
					while (player.Snapshots.size() > 20)
						player.Snapshots.pop_front();

					// If this is the first snapshot, update render position.
					if (player.Snapshots.size() == 1)
						player.RenderPosition = player.Position;

					// Check if ids match and if so update the client's data
					// tracking index.
					if (player.ID == m_ID)
					{
						if (!m_BecomeHost && !m_NewHostElected)
						{
							if (player.Connected)
								m_DataIndex = i;
							else
								Disconnect();
						}
					}
				}
				break;
			}
		case PacketType::JOIN:
			{
				// Server sends the client's unique ID (uint32).
				packet >> m_ID;
				break;
			}
		case PacketType::CLIENT_DISCONNECT:
			{
				// Update player data.
				for (auto& player : m_Players)
				{
					std::uint32_t id{};
					std::uint8_t active{};
					packet >> id >> active;

					if (player.ID == id)
					{
						player.Connected = active == 1;

						//	If the diconnected client is us, call Disconnect().
						if (player.ID == m_ID)
							Disconnect();
					}
				}
				break;
			}
		case PacketType::NEW_HOST:
			{
				std::uint32_t oldHostID{};
				std::uint32_t newHostID{};
				std::uint32_t addressNumber{};
				Port newPort{};
				packet >> oldHostID >> newHostID >> addressNumber >> newPort;

				// Update host address and port.
				m_HostIP = sf::IpAddress(addressNumber);
				m_HostPort = newPort;

				// Determine whether the client is the new host or if a new
				// host has been elected.
				if (newHostID == m_ID)
				{
					Log::PrintMsg("You're the new host.");
					m_BecomeHost = true;
				}
				else if (oldHostID == m_ID) // Prevent old host from connecting.
				{
					Log::PrintMsg("A new host has been elected.");
					m_NewHostElected = true;
				}
				else if (newHostID != m_ID && oldHostID != m_ID)
				{
					Log::PrintMsg("A new host has been elected.");
					m_Reconnect = true;
					m_NewHostElected = true;
				}

				Disconnect();
				break;
			}
		case PacketType::DUEL_REQUEST: break; //< Send only packet type.
		case PacketType::DUEL_CHOICE: break;  //< Send only packet type.
		case PacketType::DUEL_START:
			{
				std::uint32_t opponent{};
				packet >> opponent;
				auto msg = Log::Format("Duel started with player {}.",
									   opponent);
				Log::PrintMsg(msg);
				constexpr std::string_view duelOptions =
						"You can select these options:\n"
						". Press R for Rock.\n"
						". Press P for Paper.\n"
						". Press C for Succour.";

				Log::PrintMsg(duelOptions);
				break;
			}
		case PacketType::DUEL_RESULT:
			{
				std::uint8_t winStateRaw{};
				packet >> winStateRaw;

				m_WinState = static_cast<WinState>(winStateRaw);

				break;
			}
		case PacketType::PING: break; //< Send only packet type.
		case PacketType::PONG:
			{
				float echoedTime{};
				packet >> echoedTime;

				m_RTT = m_ServerTime - echoedTime;
				break;
			}
		}
	}

	InterpolatePlayers();
}

void Game::Client::Render(sf::RenderWindow& window) const
{
	for (auto& player : m_Players)
	{
		if (!player.Connected)
			continue;

		player.Shape->setPosition(player.RenderPosition);
		player.Shape->setColor(player.Colour);
		window.draw(*player.Shape);
	}
}


bool Game::Client::WantReconnection() const
{
	return m_Reconnect;
}

bool Game::Client::IsConnected() const
{
	return m_Connected;
}

void Game::Client::Disconnect()
{
	if (!m_Connected)
		return;

	m_Socket.disconnect();
	m_Socket.setBlocking(true);
	m_ID = 0;
	m_Connected = false;
	m_DataIndex = SIZE_MAX;
	m_Players.clear();
	m_WinState = WinState::NONE;
	m_ServerTime = 0.f;

	Log::PrintMsg("You have been disconnected from the server.");
}


bool Game::Client::NewHostElected() const
{
	return m_NewHostElected;
}

bool Game::Client::BecomeHost() const
{
	return m_BecomeHost;
}

void Game::Client::ClearHostFlag()
{
	m_BecomeHost = false;
}


bool Game::Client::InDuel() const
{
	if (m_DataIndex >= m_Players.size())
		return false;

	return m_Players[m_DataIndex].Duel.InDuel;
}

bool Game::Client::HasChosen() const
{
	if (m_DataIndex >= m_Players.size())
		return false;

	return m_Players[m_DataIndex].Duel.Choice != RPSChoice::NONE;
}

void Game::Client::RequestDuel(const std::uint32_t targetID)
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(PacketType::DUEL_REQUEST)
			<< targetID;

	if (m_Socket.send(packet) != sf::Socket::Status::Done)
	{
		const auto msg = Log::Format(
				"Had a problem sending over the {} packet to server!",
				ToString(PacketType::DUEL_RESULT));
		Log::PrintMsg(msg, ERROR);
	}
}

void Game::Client::SendRPS(RPSChoice choice)
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(PacketType::DUEL_CHOICE)
			<< static_cast<std::uint8_t>(choice);

	if (m_Socket.send(packet) != sf::Socket::Status::Done)
	{
		const auto msg = Log::Format(
				"Had a problem sending over the {} packet to server!",
				ToString(PacketType::DUEL_CHOICE));
		Log::PrintMsg(msg, ERROR);
	}

	const auto msg = Log::Format("You have chosen {}.", ToString(choice));
	Log::PrintMsg(msg);
}

std::optional<std::uint32_t> Game::Client::FindNearestDuelTarget(
		const float maxRange) const
{
	if (m_DataIndex >= m_Players.size())
		return std::nullopt;

	const auto& me = m_Players[m_DataIndex];
	float bestDistSq = maxRange * maxRange;
	std::optional<std::uint32_t> bestID;

	for (const auto& player : m_Players)
	{
		if (!player.Connected || player.ID == m_ID)
			continue;

		const sf::Vector2f d = player.Position - me.Position;
		const float distSq = d.x * d.x + d.y * d.y;

		if (distSq <= bestDistSq)
		{
			bestDistSq = distSq;
			bestID = player.ID;
		}
	}
	return bestID;
}

void Game::Client::SendPing()
{
	if (!m_Connected)
		return;

	sf::Packet packet;
	packet << static_cast<std::uint8_t>(PacketType::PING);
	packet << m_ServerTime;

	if (m_Socket.send(packet) != sf::Socket::Status::Done)
	{
		const auto msg = Log::Format(
				"Had a problem sending over the {} packet to server!",
				ToString(PacketType::PING));
		Log::PrintMsg(msg, ERROR);
	}

	m_LastPingSendTime = m_ServerTime;
}


std::uint32_t Game::Client::GetID() const
{
	return m_ID;
}

sf::IpAddress Game::Client::GetHostAddress() const
{
	return m_HostIP;
}

Game::Port Game::Client::GetHostPort() const
{
	return m_HostPort;
}

sf::Vector2f Game::Client::GetPosition() const
{
	if (m_DataIndex >= m_Players.size())
		return {0.f, 0.f};

	return m_Players[m_DataIndex].Position;
}

sf::Vector2f Game::Client::GetSize() const
{
	if (m_DataIndex >= m_Players.size())
		return {0.f, 0.f};

	return m_Players[m_DataIndex].Shape->GetSize();
}

Game::WinState& Game::Client::GetWinState()
{
	return m_WinState;
}

void Game::Client::PrintLatency() const
{
	if (m_RTT < 0.0001)
		return;

	const float oneWay = m_RTT * 0.5f;

	const auto msg = Log::Format(
			"Latency: {:.1f} ms RTT | {:.1f} ms one-way",
			m_RTT * 1000.f,
			oneWay * 1000.f);

	Log::PrintMsg(msg, INFO);
}

void Game::Client::InterpolatePlayers()
{
	// Interpolation delay to render behind server time.
	const float interpDelay = NetworkSettings::Get().InterpDelay;

	// Target time we want to render at.
	const float renderTime = m_ServerTime - interpDelay;

	// Interpolate every known player.
	for (auto& player : m_Players)
	{
		// If we do not have enough history, snap directly to the latest position.
		if (player.Snapshots.size() < 2)
		{
			player.RenderPosition = player.Position;
			continue;
		}

		const TransformSnapshot* older{nullptr};
		const TransformSnapshot* newer{nullptr};
		// Find the first snapshot that is newer than the render time.
		for (std::size_t i = 1; i < player.Snapshots.size(); ++i)
		{
			if (player.Snapshots[i].TimeStamp >= renderTime)
			{
				// Update the snapshot with a newer one.
				older = &player.Snapshots[i - 1];
				newer = &player.Snapshots[i];
				break;
			}
		}

		// If we could not find a valid pair, snap to the most recent snapshot.
		if (!older || !newer)
		{
			player.RenderPosition = player.Snapshots.back().Position;
			continue;
		}

		// Interpolate between the two snapshots.
		const float alpha = (renderTime - older->TimeStamp) /
							(newer->TimeStamp - older->TimeStamp);

		// Linearly interpolate between the older and newer positions.
		player.RenderPosition = older->Position +
								(newer->Position - older->Position) * alpha;
	}
}
