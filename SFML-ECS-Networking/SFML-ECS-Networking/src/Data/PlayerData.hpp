#pragma once
#include <deque>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/System/Vector2.hpp>

#include "DuelState.hpp"
#include "../Networking/RPSChoice.hpp"
#include "../UI/Sprite.hpp"
#include "../Utilities/TextureStore.hpp"

namespace Game
{
struct TransformSnapshot
{
	sf::Vector2f Position;
	float TimeStamp;
};

/// Always send and receive data over a network in this order:
/// ID,
/// Positon,
/// Colour,
/// Connected,
/// Duel State
struct PlayerData
{
	PlayerData()
	{
		Socket.setBlocking(false);
		ID = 0;
		Position = {0.f, 0.f};
		Colour = sf::Color::White;
		Connected = false;
		Shape = std::make_unique<Sprite>(TextureStore::Get());
		Shape->SetSize({40.f, 40.f});
		RenderPosition = {0.f, 0.f};
	}

	void Disconnect()
	{
		Socket.disconnect();

		// Reset fields.
		Socket.setBlocking(true);
		ID = 0;
		Position = {0.f, 0.f};
		Colour = sf::Color::White;
		Connected = false;
		Duel = DuelState();
		Snapshots.clear();
		RenderPosition = {0.f, 0.f};

		// Reset the shape.
		Shape->setPosition({0.f, 0.f});
		Shape->setColor(Colour);
	}

	// Network variables.
	std::uint32_t ID{};
	sf::Vector2f Position{};
	std::deque<TransformSnapshot> Snapshots{};
	sf::Vector2f RenderPosition{};
	sf::Color Colour{};
	bool Connected{};
	DuelState Duel{};

	// Local variables.
	sf::TcpSocket Socket{};
	std::unique_ptr<Sprite> Shape{};
};
}
