#include "Server.hpp"

#include "../Networking/NetworkSettings.h"

Game::Server::Server()
{
	m_Players.resize(NetworkSettings::Get().MaxPlayers);
	m_Generator.seed(std::random_device{}());
}

bool Game::Server::Start(const Port port)
{
	// Reset listener & selector.
	m_Listener = sf::TcpListener();
	m_Selector.clear();

	if (m_Listener.listen(port) != sf::Socket::Status::Done)
	{
		auto msg = Log::Format("Server failed to bind to port: {}!", port);
		Log::PrintMsg(msg, ERROR, SERVER);
		return false;
	}

	m_Listener.setBlocking(false);
	m_Selector.add(m_Listener);
	m_IsRunning = true;

	Log::PrintMsg("Server listening...", INFO, SERVER);
	return true;
}

void Game::Server::Update(const float dt)
{
	m_ServerTime += dt;

	// Wait briefly for socket activity.
	if (!m_Selector.wait(sf::milliseconds(10)))
		return;

	// Accept new clients.
	if (m_Selector.isReady(m_Listener))
		AcceptNewClient();

	// Read packets from players.
	for (auto& playerData : m_Players)
	{
		if (!playerData.Connected)
			continue;

		if (!m_Selector.isReady(playerData.Socket))
			continue;

		while (m_IsRunning)
		{
			sf::Packet packet;
			const auto status = playerData.Socket.receive(packet);

			if (status == sf::Socket::Status::NotReady ||
				status == sf::Socket::Status::Partial)
				break;

			if (status == sf::Socket::Status::Disconnected)
			{
				DisconnectPlayer(playerData);
				break;
			}
			if (status == sf::Socket::Status::Error)
			{
				auto msg = Log::Format(
						"Client[{}] had an error receiving a packet!",
						std::to_string(playerData.ID));
				Log::PrintMsg(msg, ERROR, SERVER);
				break;
			}

			// Only remaining case: Done.
			std::uint8_t typeRaw{};
			if (!(packet >> typeRaw))
				continue;

			const auto type = static_cast<PacketType>(typeRaw);
			if (type == PacketType::DUEL_REQUEST)
			{
				// Read the requested duel target's unique client ID.
				std::uint32_t targetID{};
				packet >> targetID;

				// Handle duel starting logic.
				if (!HandleDuelStart(playerData, targetID))
					break;
			}
			else if (type == PacketType::DUEL_CHOICE)
			{
				// Ignore choices from players not currently in a duel.
				if (!playerData.Duel.InDuel)
					break;

				// Read and store the player's Rock-Paper-Scissors choice.
				std::uint8_t rpsRaw{};
				packet >> rpsRaw;
				playerData.Duel.Choice = static_cast<RPSChoice>(rpsRaw);

				// Handle the dueling logic.
				if (!HandleDuel(playerData))
					break;
			}
			else if (type == PacketType::UPDATE_MOVEMENT)
			{
				sf::Vector2f moveOffset{};
				if (packet >> moveOffset.x >> moveOffset.y)
					playerData.Position += moveOffset * 100.f * dt;
			}
			else if (type == PacketType::PING)
			{
				float echoedTime{};
				packet >> echoedTime;

				sf::Packet pongPacket;
				pongPacket << static_cast<std::uint8_t>(PacketType::PONG);
				pongPacket << echoedTime;

				if (playerData.Socket.send(pongPacket) !=
					sf::Socket::Status::Done)
				{
					const auto msg = Log::Format(
							"Had a problem sending over the {} packet to Client[{}]!",
							ToString(PacketType::PONG),
							std::to_string(playerData.ID));

					Log::PrintMsg(msg, ERROR, SERVER);
				}
			}
			break;
		}
	}
	BroadcastStates();
}

void Game::Server::Stop()
{
	Log::PrintMsg("Stopping server...", INFO, SERVER);

	// Try and obtain a new host.
	const auto newHostData = ElectNewHost();
	// Prepare the NEW_HOST packet.
	sf::Packet hostPacket;
	hostPacket << static_cast<std::uint8_t>(PacketType::NEW_HOST);
	if (newHostData.has_value())
	{
		hostPacket << m_Host.ID
				<< newHostData->ID
				<< newHostData->Address.toInteger()
				<< newHostData->Port;
	}

	// Prepare the CLIENT_DISCONNECT packet.
	sf::Packet disPacket;
	disPacket << static_cast<std::uint8_t>(PacketType::CLIENT_DISCONNECT);
	for (const auto& player : m_Players)
		disPacket << player.ID << static_cast<std::uint8_t>(0);

	// Prepare the MESSAGE packet.
	sf::Packet msgPacket;
	msgPacket << static_cast<std::uint8_t>(PacketType::MESSAGE);
	msgPacket <<
			"You have been disconnected because of the server shutting down."
			<< static_cast<uint8_t>(WARNING);

	// Broadcast to all connected players.
	for (auto& player : m_Players)
	{
		if (!player.Connected)
			continue;

		// Send a message to the client why they have been disconnected.
		if (player.Socket.send(msgPacket) == sf::Socket::Status::Error)
		{
			auto msg = Log::Format(
					"Failed to send the {} packet to client[{}].",
					ToString(PacketType::MESSAGE),
					std::to_string(player.ID));

			Log::PrintMsg(msg, ERROR, SERVER);
		}

		// If there is a new host elect, inform the clients before
		// disconnecting them.
		if (newHostData.has_value() &&
			player.Socket.send(hostPacket) == sf::Socket::Status::Error)
		{
			auto msg = Log::Format(
					"Failed to send the {} packet to client[{}].",
					ToString(PacketType::NEW_HOST),
					std::to_string(player.ID));

			Log::PrintMsg(msg, ERROR, SERVER);
		}
		// Actually disconnect the clients.
		else if (!newHostData.has_value() &&
				 player.Socket.send(disPacket) == sf::Socket::Status::Error)
		{
			auto msg = Log::Format(
					"Failed to send the {} packet to client[{}].",
					ToString(PacketType::CLIENT_DISCONNECT),
					std::to_string(player.ID));

			Log::PrintMsg(msg, ERROR, SERVER);
		}
	}

	// Now disconnect players and clear selector.
	if (!newHostData.has_value())
		for (auto& player : m_Players)
			DisconnectPlayer(player);

	m_Listener.close();
	m_Selector.remove(m_Listener);
	m_IsRunning = false;

	Log::PrintMsg("Server has stopped.", INFO, SERVER);
}


bool Game::Server::IsRunning() const
{
	return m_IsRunning;
}


std::uint32_t Game::Server::GetHostID() const
{
	return m_Host.ID;
}

void Game::Server::UpdateHostID(const std::uint32_t id)
{
	for (const auto& player : m_Players)
	{
		if (player.ID != id)
			continue;

		// Update host data.
		m_Host.ID = player.ID;
		m_Host.Port = player.Socket.getRemotePort();
		if (player.Socket.getRemoteAddress().has_value())
			m_Host.Address = player.Socket.getRemoteAddress().value();
	}
}


sf::IpAddress Game::Server::GetServerAddress()
{
	if (sf::IpAddress::getLocalAddress().has_value())
		return sf::IpAddress::getLocalAddress().value();
	return {0, 0, 0, 0};
}

Game::Port Game::Server::GetServerPort() const
{
	return m_Host.Port;
}


void Game::Server::AcceptNewClient()
{
	// Attempt to accept a pending connection into a temporary socket.
	// Return if no pending connection.
	sf::TcpSocket incoming;
	if (m_Listener.accept(incoming) != sf::Socket::Status::Done)
		return;

	// Find a free player slot.
	for (std::size_t i = 0; i < m_Players.size(); i++)
	{
		auto& player = m_Players[i];
		if (player.Connected)
			continue; //< Check next slot.

		IntDis colourGen(0, 255);
		IntDis<std::uint32_t> idGen;

		// Move the accepted socket into the player slot.
		// And assign initial ID, position, and colour.
		player.Socket = std::move(incoming);
		player.Socket.setBlocking(false);
		player.ID = idGen(m_Generator);
		player.Position = {100.f + 200.f * static_cast<float>(i), 300.f};
		player.Colour = sf::Color(
				static_cast<std::uint8_t>(colourGen(m_Generator)),
				static_cast<std::uint8_t>(colourGen(m_Generator)),
				static_cast<std::uint8_t>(colourGen(m_Generator)));
		player.Connected = true;

		// Register the socket.
		m_Selector.add(player.Socket);

		// Send JOIN acknowledgement.
		sf::Packet packet;
		packet << static_cast<std::uint8_t>(PacketType::JOIN)
				<< player.ID;

		// Send the JOIN packet and log the result.
		const auto status = player.Socket.send(packet);
		if (status == sf::Socket::Status::Done)
		{
			const auto msg = Log::Format("Client[{}] connected!",
										 std::to_string(player.ID));
			Log::PrintMsg(msg, SUCCESS, SERVER);
		}
		else if (status == sf::Socket::Status::Error)
		{
			const auto msg = Log::Format(
					"There was an issue with sending over the {} packet to client[{}].",
					ToString(PacketType::JOIN),
					std::to_string(player.ID));

			Log::PrintMsg(msg, ERROR, SERVER);
		}

		// Send a quick state update to the clients.
		BroadcastStates();
		return;
	}


	// If no free slot, reject quietly.
	const auto msg = "Connection rejected. Server full.";
	Log::PrintMsg(msg, WARNING, SERVER);

	// Prepare the message packet.
	sf::Packet msgPacket;
	msgPacket << static_cast<std::uint8_t>(PacketType::MESSAGE);
	msgPacket << msg << static_cast<uint8_t>(WARNING);

	// Send a message to the client why they have been disconnected.
	if (incoming.send(msgPacket) == sf::Socket::Status::Error)
	{
		const auto logMsg = Log::Format(
				"Failed to send the {} packet to the rejected client.",
				ToString(PacketType::MESSAGE));

		Log::PrintMsg(logMsg, ERROR, SERVER);
	}

	incoming.disconnect();
}

void Game::Server::BroadcastStates()
{
	sf::Packet packet;
	packet << static_cast<std::uint8_t>(PacketType::UPDATE_CLIENTS);
	packet << m_ServerTime << static_cast<std::uint32_t>(m_Players.size());

	// Encode each player's state.
	for (const auto& player : m_Players)
	{
		packet << player.ID
				<< player.Position.x << player.Position.y
				<< player.Colour.toInteger()
				<< static_cast<std::uint8_t>(player.Connected ? 1 : 0)
				<< static_cast<std::uint8_t>(player.Duel.InDuel ? 1 : 0)
				<< player.Duel.OpponentID
				<< static_cast<std::uint8_t>(player.Duel.Choice);
	}

	// Broadcast to all connected players.
	for (auto& player : m_Players)
	{
		if (!player.Connected ||
			player.Socket.send(packet) != sf::Socket::Status::Error)
			continue;

		auto msg = Log::Format(
				"There was an issue with broadcasting client's[{}] data.",
				player.ID);
		Log::PrintMsg(msg, ERROR, SERVER);
	}
}

void Game::Server::DisconnectPlayer(PlayerData& player)
{
	if (!player.Connected)
		return;

	const auto msg = Log::Format("Client[{}] has disconnected!",
								 std::to_string(player.ID));
	Log::PrintMsg(msg, WARNING, SERVER);

	m_Selector.remove(player.Socket);
	player.Disconnect();
}

std::optional<Game::Server::HostData> Game::Server::ElectNewHost() const
{
	for (const auto& player : m_Players)
	{
		if (!player.Connected || player.ID == m_Host.ID)
			continue;

		// Found next best host.
		HostData newHost;
		newHost.ID = player.ID;
		newHost.Port = player.Socket.getRemotePort();
		if (player.Socket.getRemoteAddress().has_value())
			newHost.Address = player.Socket.getRemoteAddress().value();

		return newHost;
	}
	return std::nullopt;
}

Game::WinState Game::Server::ResolveRPS(const RPSChoice a, const RPSChoice b)
{
	if (a == b)
		return WinState::NONE;

	if ((a == RPSChoice::ROCK && b == RPSChoice::SCISSORS) ||
		(a == RPSChoice::PAPER && b == RPSChoice::ROCK) ||
		(a == RPSChoice::SCISSORS && b == RPSChoice::PAPER))
		return WinState::WON;
	return WinState::LOST;
}

bool Game::Server::HandleDuelStart(
		std::vector<PlayerData>::value_type& playerData,
		const std::uint32_t targetID)
{
	PlayerData* target = nullptr;
	for (auto& player : m_Players)
		if (player.Connected && player.ID == targetID)
			target = &player;

	// Exit if the target does not exist or is already in a duel.
	if (!target || target->Duel.InDuel)
		return false;

	// Update duel states for the challenger and target.
	playerData.Duel = {
		.InDuel = true,
		.OpponentID = target->ID,
		.Choice = RPSChoice::NONE
	};
	target->Duel = {
		.InDuel = true,
		.OpponentID = playerData.ID,
		.Choice = RPSChoice::NONE
	};

	// Send DUAL START packet to the challenger.
	sf::Packet startPacket;
	startPacket << static_cast<std::uint8_t>(PacketType::DUEL_START);
	startPacket << target->ID;

	if (playerData.Socket.send(startPacket) != sf::Socket::Status::Done)
	{
		const auto msg = Log::Format(
				"Had a problem sending over the {} packet to Client[{}]!",
				ToString(PacketType::DUEL_START),
				std::to_string(playerData.ID));
		Log::PrintMsg(msg, ERROR, SERVER);
	}

	// Send DUAL START packet to the challenged player.
	startPacket.clear();
	startPacket << static_cast<std::uint8_t>(PacketType::DUEL_START);
	startPacket << playerData.ID;

	if (target->Socket.send(startPacket) != sf::Socket::Status::Done)
	{
		const auto msg = Log::Format(
				"Had a problem sending over the {} packet to Client[{}]!",
				ToString(PacketType::DUEL_START),
				std::to_string(target->ID));
		Log::PrintMsg(msg, ERROR, SERVER);
	}

	// Log the duel start event.
	const auto msg = Log::Format(
			"Duel started between Client[{}] and Client[{}].",
			playerData.ID, target->ID);
	Log::PrintMsg(msg, INFO, SERVER);
	return true;
}

bool Game::Server::HandleDuel(std::vector<PlayerData>::value_type& playerData)
{
	// Locate the opponent using the stored opponent ID.
	PlayerData* opponent = nullptr;
	for (auto& player : m_Players)
		if (player.ID == playerData.Duel.OpponentID)
			opponent = &player;

	// Wait until both players have submitted a valid choice.
	if (!opponent || opponent->Duel.Choice == RPSChoice::NONE)
		return false;

	const WinState result = ResolveRPS(playerData.Duel.Choice,
									   opponent->Duel.Choice);

	// Determine the winner ID, or zero for a draw.
	std::uint32_t winnerID = 0;
	if (result == WinState::WON) //< Challenger win.
		winnerID = playerData.ID;
	else if (result == WinState::LOST) //< Opponent win.
		winnerID = opponent->ID;

	// Create a result message.
	auto resultMsg = Log::Format(
			"Duel result: Client[{}] ({} ) vs Client[{}] ({} ) -> Winner: Client[{}]",
			playerData.ID,
			ToString(playerData.Duel.Choice),
			opponent->ID,
			ToString(opponent->Duel.Choice),
			winnerID == 0 ? "Draw" : std::to_string(winnerID));

	// Prepare a MESSAGE packet with the result.
	sf::Packet msgPacket;
	msgPacket << static_cast<std::uint8_t>(PacketType::MESSAGE);
	msgPacket << resultMsg << static_cast<std::uint8_t>(SUCCESS);

	// Send the duel result message to both players.
	if (playerData.Socket.send(msgPacket) !=
		sf::Socket::Status::Done)
	{
		const auto logMsg = Log::Format(
				"Had a problem sending over the {} packet to Client[{}]!",
				ToString(PacketType::MESSAGE),
				std::to_string(playerData.ID));
		Log::PrintMsg(logMsg, ERROR, SERVER);
	}
	if (opponent->Socket.send(msgPacket) !=
		sf::Socket::Status::Done)
	{
		const auto logMsg = Log::Format(
				"Had a problem sending over the {} packet to Client[{}]!",
				ToString(PacketType::MESSAGE),
				std::to_string(opponent->ID));
		Log::PrintMsg(logMsg, ERROR, SERVER);
	}

	// Log the result to server as well.
	Log::PrintMsg(resultMsg, INFO, SERVER);

	// If no draw, send the duel results to players.
	if (result != WinState::NONE)
	{
		// Prepare a DUEl_RESULT packet.
		sf::Packet resultPacket;
		// Send WON to challenger, and LOSE to opponent.
		if (result == WinState::WON)
		{
			resultPacket << static_cast<std::uint8_t>(PacketType::DUEL_RESULT);
			resultPacket << static_cast<std::uint8_t>(result);

			// Send the duel result to both players.
			if (playerData.Socket.send(resultPacket) !=
				sf::Socket::Status::Done)
			{
				const auto logMsg = Log::Format(
						"Had a problem sending over the {} packet to Client[{}]!",
						ToString(PacketType::DUEL_RESULT),
						std::to_string(playerData.ID));
				Log::PrintMsg(logMsg, ERROR, SERVER);
			}

			resultPacket.clear();
			resultPacket << static_cast<std::uint8_t>(PacketType::DUEL_RESULT);
			resultPacket << static_cast<std::uint8_t>(WinState::LOST);
			if (opponent->Socket.send(resultPacket) !=
				sf::Socket::Status::Done)
			{
				const auto logMsg = Log::Format(
						"Had a problem sending over the {} packet to Client[{}]!",
						ToString(PacketType::DUEL_RESULT),
						std::to_string(opponent->ID));
				Log::PrintMsg(logMsg, ERROR, SERVER);
			}
		}
		// Send LOSE to challenger, and WON to opponent.
		else if (result == WinState::LOST)
		{
			resultPacket << static_cast<std::uint8_t>(PacketType::DUEL_RESULT);
			resultPacket << static_cast<std::uint8_t>(result);

			// Send the duel result to both players.
			if (playerData.Socket.send(resultPacket) !=
				sf::Socket::Status::Done)
			{
				const auto logMsg = Log::Format(
						"Had a problem sending over the {} packet to Client[{}]!",
						ToString(PacketType::DUEL_RESULT),
						std::to_string(playerData.ID));
				Log::PrintMsg(logMsg, ERROR, SERVER);
			}

			resultPacket.clear();
			resultPacket << static_cast<std::uint8_t>(PacketType::DUEL_RESULT);
			resultPacket << static_cast<std::uint8_t>(WinState::WON);
			if (opponent->Socket.send(resultPacket) !=
				sf::Socket::Status::Done)
			{
				const auto logMsg = Log::Format(
						"Had a problem sending over the {} packet to Client[{}]!",
						ToString(PacketType::DUEL_RESULT),
						std::to_string(opponent->ID));
				Log::PrintMsg(logMsg, ERROR, SERVER);
			}
		}
	}

	// Reset duel state.
	playerData.Duel = {};
	opponent->Duel = {};
	return true;
}
