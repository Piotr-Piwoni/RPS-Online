#include <fstream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include "Core/Application.hpp"
#include "Networking/NetworkSettings.h"
#include "Utilities/Log.hpp"
#include "Utilities/TextureStore.hpp"


int main()
{
	// Function definitions.
	void CreateDefaultNetworkSettings(const std::filesystem::path&);
	bool LoadNetworkSettings(const std::filesystem::path&,
									Game::NetworkSettings&);


	Game::Log::PrintMsg("Game startup...");

	// Setup network settings file.
	auto& settings = Game::NetworkSettings::Get();
	const std::filesystem::path configPath = "network-settings.init";
	if (!std::filesystem::exists(configPath))
	{
		CreateDefaultNetworkSettings(configPath);
		Game::Log::PrintMsg("Created default network-settings.init",
							Game::INFO);
	}
	LoadNetworkSettings(configPath, settings);


	// Init player texture.
	Game::TextureStore::Load(std::string(Game::SPRITES) + "player.png");

	// Prepare the window.
	const auto window = std::make_unique<sf::RenderWindow>();
	window->create(sf::VideoMode({800, 600}), "RPS Game");
	window->setFramerateLimit(0);

	const auto app = std::make_unique<Game::Application>(*window);

	sf::Texture bgTexture{std::string(Game::TEXTURES) + "arena.png"};
	sf::Sprite background{bgTexture};

	// Scale to fit the window.
	const auto textureSize = static_cast<sf::Vector2f>(bgTexture.getSize());
	const auto windowSize = static_cast<sf::Vector2f>(window->getSize());

	background.setScale({
		windowSize.x / textureSize.x,
		windowSize.y / textureSize.y
	});

	sf::Clock clock;
	while (window->isOpen())
	{
		// Calculate delta time.
		const float deltaTime = clock.restart().asSeconds();

		while (const std::optional event = window->pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window->close();
			}
			// Handle keyboard input.
			else if (const auto* keyPressed = event->getIf<
				sf::Event::KeyPressed>())
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Escape))
					window->close();

				app->HandleInput(deltaTime);
			}

			app->HandleUIEvents(event, *window);
		}

		app->Update(deltaTime);

		// Rendering.
		window->clear();

		window->draw(background);
		app->Render();

		window->display();
	}

	return 0;
}

/// Create a default network settings file.
void CreateDefaultNetworkSettings(const std::filesystem::path& path)
{
	std::ofstream file(path);
	if (!file.is_open())
		return;

	file << "port=54000\n";
	file << "max_players=3\n";
	file << "interp_delay=0.1\n";
}

/// Load the network settings file and update the NetworkSettings instance.
bool LoadNetworkSettings(const std::filesystem::path& path,
						 Game::NetworkSettings& settings)
{
	std::ifstream file(path);
	if (!file.is_open())
		return false;

	std::string line;
	while (std::getline(file, line))
	{
		// Find values after the equals sign.
		const auto sep = line.find('=');
		if (sep == std::string::npos)
			continue;

		const std::string key = line.substr(0, sep);
		const std::string value = line.substr(sep + 1);

		if (key == "port")
			settings.Port = static_cast<Game::Port>(std::stoi(value));
		else if (key == "max_players")
			settings.MaxPlayers = std::stoul(value);
		else if (key == "interp_delay")
			settings.InterpDelay = std::stof(value);
	}
	return true;
}
