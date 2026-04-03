#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "../UI/UIElement.hpp"

// Forward declaration.
namespace sf
{
class RenderWindow;
class Event;
}

namespace Game
{
class UIManager
{
public:
	UIManager();

	/// Create a UI element and keep track of it.
	template<typename T, typename... Args> requires std::derived_from<
		T, UIElement>
	T* Create(Args&&... args);

	/// Find a UI element by type and name.
	template<typename T> requires std::derived_from<T, UIElement>
	T* Find(std::string_view name);

	/// Find all UI elements of a certain type.
	template<typename T> requires std::derived_from<T, UIElement>
	std::vector<T*> FindAll();

	/// Register an already-created UI element.
	void Register(std::unique_ptr<UIElement> element);
	/// Remove UI element by raw pointer.
	void Remove(const UIElement* element);

	void HandleEvent(const std::optional<sf::Event>& event,
					 const sf::RenderWindow& window) const;
	void Update(float dt);
	void Render(sf::RenderTarget& target) const;

private:
	std::vector<std::unique_ptr<UIElement>> m_Elements{};
};

template<typename T, typename... Args> requires std::derived_from<T, UIElement>
T* UIManager::Create(Args&&... args)
{
	// Construct element by passing in this Ui Manager and the rest of the arguments.
	auto ptr = std::make_unique<T>(this, std::forward<Args>(args)...);
	T* raw = ptr.get();
	m_Elements.push_back(std::move(ptr));
	return raw;
}

template<typename T> requires std::derived_from<T, UIElement>
T* UIManager::Find(std::string_view name)
{
	for (auto& uElement : m_Elements)
		if (auto tElement = dynamic_cast<T*>(uElement.get());
			tElement && tElement->GetName() == name)
			return tElement;

	const std::string msg = Log::Format(
			"Failed to find an element of type<{}> with the name {}!",
			typeid(T).name(),
			name);

	Log::PrintMsg(msg, ERROR);
	return nullptr;
}

template<typename T> requires std::derived_from<T, UIElement>
std::vector<T*> UIManager::FindAll()
{
	std::vector<T*> outList;
	outList.reserve(m_Elements.size());

	for (auto& uElement : m_Elements)
		if (auto tElement = dynamic_cast<T*>(uElement.get()))
			outList.push_back(tElement);
	return outList;
}
}
