#pragma once
#include "Client.hpp"
#include "Server.hpp"
#include "../Managers/UIManager.hpp"
#include "../UI/UI.hpp"
#include "../Utilities/Log.hpp"

namespace Game
{
class Application
{
public:
	enum class GameState : std::uint8_t { MAIN_MENU, GAME, DUEL };

public:
	explicit Application(sf::RenderWindow& window);

	void HandleUIEvents(const std::optional<sf::Event>& event,
						const sf::RenderWindow& window) const;
	void HandleInput(float dt);
	void Update(float dt);
	void Render() const;

private:
	bool HostServer();
	bool IsServerRunning() const;
	void HandleDueling();
	void StopServer();

private:
	sf::RenderWindow* m_Window{};
	Client m_Client{};
	std::unique_ptr<Server> m_HostServer{};

	sf::IpAddress m_Ip{127, 0, 0, 1};
	Port m_Port{54000};
	bool m_IsHost{false};

	UIManager m_UiManager{};
	GameState m_CurrentState{GameState::MAIN_MENU};
	Canvas* m_UI{nullptr};
	Canvas* m_RPSButtons{nullptr};
	Label* m_DuelLabel{nullptr};

	const float DUEL_RANGE{80.f};
	std::optional<std::uint32_t> m_DuelTarget{std::nullopt};

	bool m_PingToggle{false};
};
}
