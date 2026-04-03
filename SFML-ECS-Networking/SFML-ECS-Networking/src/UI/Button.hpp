#pragma once
#include <functional>
#include <string>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include "UIElement.hpp"

namespace Game
{
class Button : public UIElement
{
	enum class ButtonState : std::uint8_t { NORMAL, HOVERED, PRESSED };

public:
	explicit Button(UIManager* manager = nullptr,
					const sf::Vector2f size = {100.f, 30.f},
					const sf::Vector2f position = {0.f, 0.f},
					const std::string& text = "Text",
					const std::string_view name = "Button",
					UIElement* parent = nullptr):
		UIElement{manager, name, parent},
		m_Text{LoadFontAndReturnText(text)}
	{
		// Setup shape.
		m_Shape.setSize(size);
		setPosition(position);
		m_Shape.setFillColor(m_MainColour);

		UpdateTextLayout();
	}


	void SetCallback(const std::function<void()>& callback)
	{
		m_Callback = callback;
	}

	void HandleEvent(const std::optional<sf::Event>& event,
					 const sf::RenderWindow& window) override
	{
		if (!event.has_value() || !m_IsVisible)
			return;

		sf::Vector2f mousePosition = {-1.f, -1.f};
		const sf::FloatRect bounds = GetGlobalTransform().transformRect(
				m_Shape.getLocalBounds());
		// Handle the pressed event.
		if (const auto* pressed = event->getIf<
			sf::Event::MouseButtonPressed>())
		{
			mousePosition = window.mapPixelToCoords(pressed->position);
			if (pressed->button == sf::Mouse::Button::Left &&
				bounds.contains(mousePosition))
				m_State = ButtonState::PRESSED;
		}
		// Handle the released event.
		else if (const auto* released = event->getIf<
			sf::Event::MouseButtonReleased>())
		{
			mousePosition = window.mapPixelToCoords(released->position);
			if (released->button == sf::Mouse::Button::Left)
			{
				// Trigger callback on release.
				if (m_State == ButtonState::PRESSED &&
					bounds.contains(mousePosition) &&
					m_Callback)
				{
					m_Callback();
				}
				m_State = ButtonState::NORMAL;
			}
		}

		// Check if hovering.
		if (mousePosition.x == -1.f && mousePosition.y == -1.f)
		{
			mousePosition = sf::Vector2f(sf::Mouse::getPosition(window));
			mousePosition = window.mapPixelToCoords(
					static_cast<sf::Vector2i>(mousePosition));
		}

		// Update state.
		if (bounds.contains(mousePosition) && m_State != ButtonState::PRESSED)
			m_State = ButtonState::HOVERED;
		else if (m_State != ButtonState::PRESSED)
			m_State = ButtonState::NORMAL;
	}

	void Update(float dt) override
	{
		if (!m_IsVisible)
			return;

		// Update colour based on state.
		switch (m_State)
		{
		case ButtonState::NORMAL:
			m_Shape.setFillColor(m_MainColour);
			break;
		case ButtonState::HOVERED:
			m_Shape.setFillColor(m_HoverColour);
			break;
		case ButtonState::PRESSED:
			m_Shape.setFillColor(m_PressedColour);
			break;
		}
	}

	/// Colour Getters.
	sf::Color GetFillColour() const { return m_MainColour; }
	sf::Color GetHoverColour() const { return m_HoverColour; }
	sf::Color GetPressedColour() const { return m_PressedColour; }
	sf::Color GetTextColour() const { return m_Text.getFillColor(); }
	/// Colour Setters.
	void SetFillColour(const sf::Color colour) { m_MainColour = colour; }
	void SetHoverColour(const sf::Color colour) { m_HoverColour = colour; }
	void SetPressedColour(const sf::Color colour) { m_PressedColour = colour; }
	void SetTextColour(const sf::Color colour) { m_Text.setFillColor(colour); }

	/// @return The button's text string.
	sf::String GetText() const { return m_Text.getString(); }
	/// Set the button's text string.
	void SetText(const std::string& text)
	{
		m_Text.setString(text);
		UpdateTextLayout();
	}

	/// @return The button's font.
	sf::Font GetFont() const { return m_Font; }
	/// Set the button's font.
	void SetFont(const std::string& fontType)
	{
		TrySetFont(fontType);
		UpdateTextLayout();
	}

	/// Get the button's size.
	sf::Vector2f GetSize() const { return m_Shape.getSize(); }
	/// Set the button's size.
	void SetSize(const sf::Vector2f& size)
	{
		m_Shape.setSize(size);
		UpdateTextLayout();
	}

protected:
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override
	{
		if (!CanDraw())
			return;

		ApplyAllTransforms(states);

		target.draw(m_Shape, states);
		target.draw(m_Text, states);
	}

	/// Try and set the desired font.
	/// If failed, fall back to the default font.
	void TrySetFont(const std::string& fontFile)
	{
		// Try the desired font.
		if (m_Font.openFromFile(std::string(FONTS) + fontFile))
			return;

		// If failed, log and fallback to default.
		Log::PrintMsg(Log::Format("Failed to load {} font for the button!",
								  fontFile), ERROR);

		// Fallback only if provided font is not already the default.
		/* INFO:
		 * It's to prevent a double log issue if the font we tried to set and
		 * failed was the default. */
		if (fontFile != DEFAULT_FONT &&
			!m_Font.openFromFile(std::string(FONTS) +
								 std::string(DEFAULT_FONT)))
		{
			Log::PrintMsg(Log::Format("Failed to load {} font for the button!",
									  DEFAULT_FONT), ERROR);
		}
	}

	/// Helper function to load font and construct sf::Text.
	sf::Text LoadFontAndReturnText(const std::string& text)
	{
		TrySetFont(std::string(DEFAULT_FONT));
		sf::Text t(m_Font, text, 18);
		t.setFillColor(sf::Color::Black);
		return t;
	}

	/// Update text to ensure its laid out properly in the button.
	void UpdateTextLayout()
	{
		const sf::Vector2f buttonSize = m_Shape.getSize();
		sf::FloatRect textBounds = m_Text.getLocalBounds();

		// Ensure the text fits in side the button.
		unsigned int charSize = m_Text.getCharacterSize();
		while ((textBounds.size.x > buttonSize.x || textBounds.size.y >
				buttonSize.y) && charSize > 1)
		{
			charSize--;
			m_Text.setCharacterSize(charSize);
			textBounds = m_Text.getLocalBounds();
		}

		// Center text.
		m_Text.setOrigin({
			textBounds.position.x + textBounds.size.x / 2.f,
			textBounds.position.y + textBounds.size.y / 2.f
		});
		m_Text.setPosition(buttonSize / 2.f);
	}

protected:
	ButtonState m_State{ButtonState::NORMAL};
	std::function<void()> m_Callback{};

	sf::RectangleShape m_Shape{};
	sf::Font m_Font{};
	sf::Text m_Text;
	sf::Color m_MainColour{196, 196, 196};
	sf::Color m_HoverColour{214, 214, 214};
	sf::Color m_PressedColour{173, 173, 173};
};
}
