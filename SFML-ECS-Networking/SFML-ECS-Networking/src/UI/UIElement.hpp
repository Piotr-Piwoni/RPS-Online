#pragma once
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Window/Event.hpp>
#include "../Utilities/Log.hpp"
#include "../Utilities/Paths.hpp"

namespace Game
{
// Forward declaration.
class UIManager;

constexpr std::string_view DEFAULT_FONT = "arial.ttf";

class UIElement : public sf::Drawable, public sf::Transformable
{
public:
	explicit UIElement(UIManager* manager = nullptr,
					   std::string_view name = "UI Element",
					   UIElement* parent = nullptr);

	~UIElement() override = default;


	/// Handle input events.
	virtual void HandleEvent(const std::optional<sf::Event>& event,
							 const sf::RenderWindow& window) = 0;
	/// Update UI state and visuals.
	virtual void Update(float dt) = 0;


	/// @return The element's visibility status.
	bool IsVisible() const;
	/// Sets the element's visibility status.
	void SetVisible(bool val);

	/// @return The element's name.
	std::string_view GetName() const;
	/// Sets the element's name.
	void SetName(std::string_view name);

	/// @return The element's parent.
	UIElement* GetParent() const;
	/// Set the parent element.
	void SetParent(UIElement* parent);

	/// Find a child by type and name.
	template<typename T> requires std::derived_from<T, UIElement>
	T* Find(std::string_view name);
	/// Find all children of a certain type.
	template<typename T> requires std::derived_from<T, UIElement>
	std::vector<T*> FindAll();

	/// Destroy the element and remove it from the UI Manager if it exists.
	void Destroy() const;

	sf::Transform GetGlobalTransform() const;


	/// @brief Set the UI Manager if it doesn't have one.
	/// @warning To be used exclusively by the UI Manager!
	void SetUIManager(UIManager* manager);

protected:
	void draw(sf::RenderTarget& target,
			  sf::RenderStates states = sf::RenderStates::Default)
	const override = 0;

	/// A helper function that demerits if the element can be rendered or not.
	bool CanDraw() const;
	/// A helper function for applying transforms.
	void ApplyAllTransforms(sf::RenderStates& states) const;

protected:
	UIManager* m_Manager{nullptr};
	UIElement* m_Parent{nullptr};
	std::vector<UIElement*> m_Children{};
	bool m_IsVisible{true};
	std::string m_Name{};
};


template<typename T> requires std::derived_from<T, UIElement>
T* UIElement::Find(std::string_view name)
{
	// Check self first.
	if (auto child = dynamic_cast<T*>(this); child && child->GetName() == name)
		return child;

	// Search recursively in children.
	for (auto* child : m_Children)
		if (auto found = child->Find<T>(name))
			return found;

	const std::string msg = Log::Format(
			"Failed to find a child of type<{}> with the name {}!",
			typeid(T).name(),
			name);

	Log::PrintMsg(msg, ERROR);
	return nullptr;
}

template<typename T> requires std::derived_from<T, UIElement>
std::vector<T*> UIElement::FindAll()
{
	std::vector<T*> outList;
	outList.reserve(m_Children.size());

	if (auto child = dynamic_cast<T*>(this))
		outList.push_back(child);

	// Search recursively in children.
	for (auto* child : m_Children)
	{
		auto childList = child->FindAll<T>();
		outList.insert(outList.end(), childList.begin(), childList.end());
	}

	return outList;
}
}
