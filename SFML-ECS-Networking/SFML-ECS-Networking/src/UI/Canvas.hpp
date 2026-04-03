#pragma once
#include "UIElement.hpp"

namespace Game
{
class Canvas : public UIElement
{
public:
	explicit Canvas(UIManager* manager = nullptr,
					const sf::Vector2f position = {0.f, 0.f},
					const std::string_view name = "Canvas",
					UIElement* parent = nullptr):
		UIElement{manager, name, parent}
	{
		setPosition(position);
	}

	void HandleEvent(const std::optional<sf::Event>& event,
					 const sf::RenderWindow& window) override {}

	void Update(float dt) override {}

protected:
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override
	{
		if (!CanDraw())
			return;

		ApplyAllTransforms(states);
	}
};
}
