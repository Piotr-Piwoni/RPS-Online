#include "UIElement.hpp"

#include "../Managers/UIManager.hpp"


Game::UIElement::UIElement(UIManager* manager,
						   const std::string_view name,
						   UIElement* parent): m_Manager{manager},
											   m_Name{name}
{
	m_Children.reserve(20);
	SetParent(parent);
}

void Game::UIElement::Destroy() const
{
	if (m_Manager)
		m_Manager->Remove(this);
}

sf::Transform Game::UIElement::GetGlobalTransform() const
{
	if (m_Parent)
		return m_Parent->GetGlobalTransform() * getTransform();
	else
		return getTransform();
}

bool Game::UIElement::IsVisible() const
{
	return m_IsVisible;
}

void Game::UIElement::SetVisible(const bool val)
{
	m_IsVisible = val;
}

std::string_view Game::UIElement::GetName() const
{
	return m_Name;
}

void Game::UIElement::SetName(const std::string_view name)
{
	m_Name = name;
}

Game::UIElement* Game::UIElement::GetParent() const
{
	return m_Parent;
}

void Game::UIElement::SetParent(UIElement* parent)
{
	// If the parent is the same as the current one, do nothing.
	if (m_Parent == parent)
		return;

	// Remove from the current parent if it exists.
	if (m_Parent)
	{
		std::erase_if(m_Parent->m_Children,
					  [this](const UIElement* ptr)
					  {
						  return ptr == this;
					  });
		m_Parent = nullptr;
	}

	// Attach to the new parent if provided.
	if (parent)
	{
		m_Parent = parent;
		parent->m_Children.push_back(this);
	}
}

void Game::UIElement::SetUIManager(UIManager* manager)
{
	m_Manager = manager;
}

bool Game::UIElement::CanDraw() const
{
	// Can draw only if this element is visible and either has no parent or
	// the parent is visible.
	return m_IsVisible && (!m_Parent || m_Parent->IsVisible());
}

void Game::UIElement::ApplyAllTransforms(sf::RenderStates& states) const
{
	sf::Transform totalTransform{};

	// Recursively add up the transforms.
	auto current = this;
	while (current)
	{
		totalTransform = current->getTransform() * totalTransform;
		current = current->GetParent();
	}

	// Apply the accumulated transform.
	states.transform *= totalTransform;
}
