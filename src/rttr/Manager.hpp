#pragma once
#include "rttr/Class.hpp"

#include <unordered_map>
#include <string>
#include <typeindex>
#include <memory>

namespace rttr
{

class ClassBase;

class Manager
{
public:
	Manager() = default;
	~Manager() = default;

	template <typename T>
	Class<T>& RegisterClass(const std::string& name)
	{
		auto it = m_classes.find(typeid(T));
		if (it == m_classes.end())
		{
			auto emplaceResult = m_classes.emplace(typeid(T), std::make_unique<Class<T>>(name));
			return *static_cast<Class<T>*>(emplaceResult.first->second.get());
		}
		else
		{
			return *static_cast<Class<T>*>(it->second.get());
		}
	}

	template <typename T>
	Class<T>* GetClass() const
	{
		auto it = m_classes.find(typeid(T));
		if (it != m_classes.end())
		{
			return static_cast<Class<T>*>(it->second.get());
		}

		return nullptr;
	}

	ClassBase* GetClass(const std::type_index& typeIndex) const
	{
		auto it = m_classes.find(typeIndex);
		if (it != m_classes.end())
		{
			return it->second.get();
		}

		return nullptr;
	}

	static Manager& GetRTTRManager()
	{
		static Manager s_manager;
		return s_manager;
	}

private:
	std::unordered_map<std::type_index, std::unique_ptr<ClassBase>> m_classes;
};

template <typename T>
Class<T>& DeclClass(const std::string& name)
{
	return Manager::GetRTTRManager().RegisterClass<T>(name);
}

} // namespace rttr
