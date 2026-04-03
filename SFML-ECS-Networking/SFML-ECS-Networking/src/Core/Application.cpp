#include "Application.hpp"

#include "../Networking/NetworkSettings.h"
namespace Keyboard = sf::Keyboard;

Game::Application::Application(sf::RenderWindow& window) : m_Window{&window}
{
	// Load network settings.
	m_Port = NetworkSettings::Get().Port;

	// Construct UI Elements.
	// Canvas.
	m_UI = m_UiManager.Create<Canvas>(
			sf::Vector2f{0.f, 0.f},
			"MainMenuUI");
	m_RPSButtons = m_UiManager.Create<Canvas>(
			sf::Vector2f{0.f, 0.f},
			"RPSButtons");

	// Buttons.
	const auto hostBtn = m_UiManager.Create<Button>(
			sf::Vector2f{100.f, 30.f},
			sf::Vector2f{20.f, 20.f},
			"Host", "HostBtn", m_UI);
	const auto joinBtn = m_UiManager.Create<Button>(
			sf::Vector2f{100.f, 30.f},
			sf::Vector2f{105.f, 0.f},
			"Join", "JoinBtn", hostBtn);
	const auto returnBtn = m_UiManager.Create<Button>(
			sf::Vector2f{100.f, 30.f},
			sf::Vector2f{
				static_cast<float>(m_Window->getSize().x) - 20.f - 100.0f,
				20.f
			},
			"Return", "ReturnBtn", m_UI);

	// RPS buttons.
	const auto rockBtn = m_UiManager.Create<Button>(
			sf::Vector2f{75.f, 25.f},
			sf::Vector2f{0.f, 0.f},
			"Rock", "RockBtn", m_RPSButtons);
	const auto paperBtn = m_UiManager.Create<Button>(
			sf::Vector2f{75.f, 25.f},
			sf::Vector2f{80.f, 0.f},
			"Paper", "PaperBtn", m_RPSButtons);
	const auto scissorsBtn = m_UiManager.Create<Button>(
			sf::Vector2f{75.f, 25.f},
			sf::Vector2f{160.f, 0.f},
			"Scissors", "ScissorsBtn", m_RPSButtons);

	// Labels.
	m_DuelLabel = m_UiManager.Create<Label>("Press Q to duel!");

	// Modify the Ui Elements.
	m_RPSButtons->move({50.f, 40.f});
	m_RPSButtons->SetVisible(false);

	hostBtn->SetCallback([this]()
	{
		if (!m_IsHost && !IsServerRunning())
			HostServer();
		if (m_IsHost && IsServerRunning() && m_CurrentState != GameState::GAME)
			m_CurrentState = GameState::GAME;
	});
	joinBtn->SetCallback([this]()
	{
		if (!m_Client.IsConnected())
			m_Client.Connect(m_Ip, m_Port);
		if (m_Client.IsConnected() && m_CurrentState != GameState::GAME)
			m_CurrentState = GameState::GAME;
	});
	returnBtn->SetCallback([this]()
	{
		// Disconnect client if connected.
		if (m_Client.IsConnected() && !m_IsHost)
			m_Client.Disconnect();

		// Stop the server if running.
		if (m_HostServer)
			StopServer();

		m_CurrentState = GameState::MAIN_MENU;
	});

	rockBtn->SetCallback([this]()
	{
		if (m_Client.InDuel() && !m_Client.HasChosen())
			m_Client.SendRPS(RPSChoice::ROCK);
	});
	paperBtn->SetCallback([this]()
	{
		if (m_Client.InDuel() && !m_Client.HasChosen())
			m_Client.SendRPS(RPSChoice::PAPER);
	});
	scissorsBtn->SetCallback([this]()
	{
		if (m_Client.InDuel() && !m_Client.HasChosen())
			m_Client.SendRPS(RPSChoice::SCISSORS);
	});

	m_DuelLabel->SetColour(sf::Color::White);
	m_DuelLabel->SetVisible(false);
}

void Game::Application::HandleUIEvents(const std::optional<sf::Event>& event,
									   const sf::RenderWindow& window) const
{
	m_UiManager.HandleEvent(event, window);
}

void Game::Application::HandleInput(const float dt)
{
	if (Keyboard::isKeyPressed(Keyboard::Scan::Num1) && !m_IsHost &&
		!m_HostServer)
		HostServer();
	else if (Keyboard::isKeyPressed(Keyboard::Scan::Num2) &&
			 !m_Client.IsConnected())
		m_Client.Connect(m_Ip, m_Port);


	// Ignore rest of function if the client isn't connected.
	if (!m_Client.IsConnected())
		return;

	// Movement.
	if (!m_Client.InDuel())
	{
		sf::Vector2f moveInput{0.f, 0.f};
		if (Keyboard::isKeyPressed(Keyboard::Scan::W))
			moveInput.y -= 1.f;
		if (Keyboard::isKeyPressed(Keyboard::Scan::S))
			moveInput.y += 1.f;
		if (Keyboard::isKeyPressed(Keyboard::Scan::A))
			moveInput.x -= 1.f;
		if (Keyboard::isKeyPressed(Keyboard::Scan::D))
			moveInput.x += 1.f;

		m_Client.SendInput(moveInput);
	}

	// Start a duel.
	if (m_DuelTarget.has_value() && !m_Client.InDuel() &&
		Keyboard::isKeyPressed(Keyboard::Scan::Q))
	{
		m_Client.RequestDuel(*m_DuelTarget);
		m_DuelTarget.reset();
	}

	// Choose an option during the duel.
	if (m_Client.InDuel() && !m_Client.HasChosen())
	{
		if (Keyboard::isKeyPressed(Keyboard::Scan::R))
			m_Client.SendRPS(RPSChoice::ROCK);
		if (Keyboard::isKeyPressed(Keyboard::Scan::P))
			m_Client.SendRPS(RPSChoice::PAPER);
		if (Keyboard::isKeyPressed(Keyboard::Scan::C))
			m_Client.SendRPS(RPSChoice::SCISSORS);
	}

	// A key for toggling measuring latency.
	if (Keyboard::isKeyPressed(Keyboard::Scan::V))
		m_PingToggle = !m_PingToggle;

	// Ask the client to ping the server.
	if (m_PingToggle)
		m_Client.SendPing();
}

void Game::Application::Update(const float dt)
{
	// If the server is running, and it doesn't know who the host is,
	// pass it the host ID.
	if (IsServerRunning() && m_HostServer->GetHostID() == 0)
		m_HostServer->UpdateHostID(m_Client.GetID());

	// Update the UI.
	m_UiManager.Update(dt);

	// Update client.
	m_Client.UpdateState();

	if (m_PingToggle)
		m_Client.PrintLatency();

	// If a host change is happening, update the IP address and port if
	// not already done so.
	if ((m_Client.BecomeHost() || m_Client.NewHostElected()) &&
		(m_Ip != m_Client.GetHostAddress() || m_Port != m_Client.GetHostPort()))
	{
		m_Ip = m_Client.GetHostAddress();
		m_Port = m_Client.GetHostPort();
	}

	switch (m_CurrentState)
	{
	case GameState::MAIN_MENU:
		{
			const auto list = m_UI->FindAll<Button>();
			for (const auto button : list)
				button->SetVisible(button->GetName() != "ReturnBtn");

			m_DuelLabel->SetVisible(false);
			m_RPSButtons->SetVisible(false);

			// Try to connect self if self-hosting.
			if (m_IsHost && IsServerRunning() && !m_Client.IsConnected())
			{
				m_Client.Connect(m_Ip, m_Port);
				m_CurrentState = GameState::GAME;
			}
			break;
		}
	case GameState::GAME:
		{
			const auto list = m_UI->FindAll<Button>();
			for (const auto button : list)
				button->SetVisible(button->GetName() == "ReturnBtn");

			// If the client is the new host and no server is running,
			// host the server.
			if (m_Client.BecomeHost() && !IsServerRunning())
				HostServer();

			// If client isn't connected, but they want to reconnect,
			// imminently update the address & port and connect to server.
			if (m_Client.WantReconnection() && !m_Client.IsConnected())
				m_Client.Connect(m_Ip, m_Port);

			// Kick the client to the main menu if it is not connected
			// or if it needs to handle a host change.
			if (!m_Client.IsConnected() &&
				!m_Client.NewHostElected() &&
				!m_Client.BecomeHost() &&
				!m_Client.WantReconnection())
			{
				m_CurrentState = GameState::MAIN_MENU;
			}

			HandleDueling();

			if (m_Client.GetWinState() == WinState::LOST)
			{
				if (m_IsHost)
				{
					StopServer();
					m_CurrentState = GameState::MAIN_MENU;
				}
				else
					m_Client.Disconnect();
			}
			break;
		}
	case GameState::DUEL:
		break;
	}

	// Update server.
	if (m_IsHost && IsServerRunning())
		m_HostServer->Update(dt);
}

void Game::Application::Render() const
{
	// Draw the UI.
	m_UiManager.Render(*m_Window);

	m_Client.Render(*m_Window);
}


bool Game::Application::HostServer()
{
	Log::PrintMsg("Starting local server...");

	// Only if NULL, create a pointer.
	if (!m_HostServer)
		m_HostServer = std::make_unique<Server>();

	if (!m_HostServer->Start(m_Port))
	{
		Log::PrintMsg("Failed to start local server!", ERROR);
		return false;
	}
	Log::PrintMsg("Local server is up and running!", SUCCESS);

	m_Client.ClearHostFlag();
	m_IsHost = true;
	return true;
}

bool Game::Application::IsServerRunning() const
{
	return m_HostServer && m_HostServer->IsRunning();
}

void Game::Application::HandleDueling()
{
	// In game check for valid duel target.
	if (!m_Client.InDuel())
		m_DuelTarget = m_Client.FindNearestDuelTarget(DUEL_RANGE);

	// Update the duel label's visibility based on the duel target information
	// and if the client is in a duel.
	m_DuelLabel->SetVisible(!m_Client.InDuel() &&
							m_DuelTarget.has_value());

	// Move the duel label with client.
	if (m_DuelTarget.has_value())
	{
		const auto clientSize = m_Client.GetSize();
		sf::Vector2f offset = clientSize / 2.f;
		offset.x += 10.f;
		offset.y += 10.f;

		m_DuelLabel->setPosition(m_Client.GetPosition() - offset);
	}

	// Update the RPS buttons if the client is in a duel and hasn't chosen yet.
	if (m_Client.InDuel() && !m_Client.HasChosen())
	{
		m_RPSButtons->SetVisible(true);

		const auto clientSize = m_Client.GetSize();
		sf::Vector2f offset = clientSize / 2.f;
		offset.x += 80.f;
		offset.y += 10.f;

		m_RPSButtons->setPosition(m_Client.GetPosition() - offset);
	}
	else
		m_RPSButtons->SetVisible(false);
}

void Game::Application::StopServer()
{
	if (!IsServerRunning())
		return;

	m_HostServer->Stop();
	m_HostServer.reset();
	m_IsHost = false;
}
