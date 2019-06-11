#pragma once
#include "rttr/Property.hpp"
#include "rttr/Aliases.hpp"

#include <unordered_map>
#include <string>
#include <memory>

namespace rttr
{

template <typename T>
class Class
{
public:
	Class(const std::string& name)
		: m_name(name)
	{}

	template <typename Signature>
	Class& DeclProperty(const std::string& name, Signature signature)
	{
		auto property = std::make_unique<Property<T, typename ExtractValueType<Signature>::type, Signature>>(name, signature);
		std::unique_ptr<BaseProperty<T>> baseProperty(std::move(property));

		m_properties.emplace(std::move(name), std::move(baseProperty));

		return *this;
	}

private:
	const std::string m_name;
	std::unordered_map<std::string, std::unique_ptr<BaseProperty<T>>> m_properties;
};

} // namespace rttr
