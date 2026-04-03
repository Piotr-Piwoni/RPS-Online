#pragma once
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

#include "UIElement.hpp"

namespace Game
{
class Label : public UIElement
{
public:
	explicit Label(UIManager* manager = nullptr,
				   const std::string& text = "Text",
				   const sf::Vector2f position = {0.f, 0.f},
				   const std::string_view name = "Label",
				   UIElement* parent = nullptr):
		UIElement{manager, name, parent},
		m_Text{LoadFontAndReturnText(text)}
	{
		setPosition(position);
	}


	void HandleEvent(const std::optional<sf::Event>& event,
					 const sf::RenderWindow& window) override {}

	void Update(float dt) override {}


	/// Get the Label's text colour.
	sf::Color GetColour() const { return m_Text.getFillColor(); }
	/// Set text colour.
	void SetColour(const sf::Color colour) { m_Text.setFillColor(colour); }

	/// @return The Label's text string.
	const sf::String& GetText() const { return m_Text.getString(); }
	/// Set the Label's text string.
	void SetText(const std::string& text) { m_Text.setString(text); }

	/// @return The Label's font.
	const sf::Font& GetFont() const { return m_Font; }
	/// Set the Label's font.
	void SetFont(const std::string& fontType) { TrySetFont(fontType); }

protected:
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override
	{
		if (!CanDraw())
			return;

		ApplyAllTransforms(states);

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
		auto msg = Log::Format("Failed to load {} font for the {}!",
							   DEFAULT_FONT, typeid(*this).name());
		Log::PrintMsg(msg, ERROR);

		// Fallback only if provided font is not already the default.
		/* INFO:
		 * It's to prevent a double log issue if the font we tried to set and
		 * failed was the default. */
		if (fontFile != DEFAULT_FONT &&
			!m_Font.openFromFile(std::string(FONTS) +
								 std::string(DEFAULT_FONT)))
		{
			msg = Log::Format("Failed to load {} font for the {}!",
							  DEFAULT_FONT, typeid(*this).name());
			Log::PrintMsg(msg, ERROR);
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

protected:
	sf::Font m_Font{};
	sf::Text m_Text;
};
}
