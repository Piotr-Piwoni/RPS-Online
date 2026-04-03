#include "UIManager.hpp"

Game::UIManager::UIManager()
{
	m_Elements.reserve(30);
}

void Game::UIManager::Register(std::unique_ptr<UIElement> element)
{
	element->SetUIManager(this);
	m_Elements.push_back(std::move(element));
}

void Game::UIManager::Remove(const UIElement* element)
{
	std::erase_if(m_Elements,
				  [element](auto& e) { return e.get() == element; });
}

void Game::UIManager::HandleEvent(const std::optional<sf::Event>& event,
								  const sf::RenderWindow& window) const
{
	for (const auto& element : m_Elements)
		element->HandleEvent(event, window);
}

void Game::UIManager::Update(const float dt)
{
	for (const auto& element : m_Elements)
		element->Update(dt);
}

void Game::UIManager::Render(sf::RenderTarget& target) const
{
	for (auto& element : m_Elements)
		target.draw(*element);
}
