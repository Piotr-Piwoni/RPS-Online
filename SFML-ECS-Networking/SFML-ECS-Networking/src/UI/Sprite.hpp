#pragma once
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace Game
{
class Sprite : public sf::Sprite
{
public:
	explicit Sprite(const sf::Texture& texture) : sf::Sprite(texture) {}

	Sprite(const sf::Texture& texture,
		   const sf::IntRect& rectangle) : sf::Sprite(texture, rectangle) {}

	void SetSize(const sf::Vector2f& size)
	{
		SetSize(size.x, size.y);
	}

	void SetSize(const float width, const float height)
	{
		const sf::Texture& texture = getTexture();
		const sf::Vector2u textureSize = texture.getSize();

		float scaleX = width / static_cast<float>(textureSize.x);
		float scaleY = height / static_cast<float>(textureSize.y);

		setScale({scaleX, scaleY});
	}

	sf::Vector2f GetSize() const
	{
		const sf::Texture& texture = getTexture();
		const sf::Vector2u textureSize = texture.getSize();
		const sf::Vector2f scale = getScale();

		return {
			static_cast<float>(textureSize.x) * scale.x,
			static_cast<float>(textureSize.y) * scale.y
		};
	}
};
}
