#pragma once
#include <SFML/Graphics/Texture.hpp>

namespace Game
{
class TextureStore
{
public:
	static bool Load(const std::string& path)
	{
		return GetTexture().loadFromFile(path);
	}

	static sf::Texture& Get()
	{
		return GetTexture();
	}

private:
	static sf::Texture& GetTexture()
	{
		static sf::Texture texture;
		return texture;
	}
};
}
