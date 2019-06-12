#pragma once
#include "rttr/Property.hpp"
#include "rttr/Aliases.hpp"

#include <unordered_map>
#include <string>
#include <memory>
#include <cassert>

namespace rttr
{

class ClassBase
{
public:
	ClassBase(const std::string& name)
		: m_name(name)
	{}

	const std::unordered_map<std::string, std::unique_ptr<BaseProperty>>& GetProperties() const
	{
		return m_properties;
	}

protected:
	void AddProperty(const std::string& name, std::unique_ptr<BaseProperty>&& property)
	{
		auto it = m_properties.find(name);
		assert(it == m_properties.end());

		m_properties.emplace(std::move(name), std::move(property));
	}

protected:
	const std::string m_name;
	std::unordered_map<std::string, std::unique_ptr<BaseProperty>> m_properties;
};

template <typename T>
class Class
	: public ClassBase
{
public:
	Class(const std::string& name)
		: ClassBase(name)
	{}

	template <typename Signature>
	Class& DeclProperty(const std::string& name, Signature signature)
	{
		auto property = std::make_unique<MemberProperty<T, typename ExtractValueType<Signature>::type, Signature>>(name, signature);
		std::unique_ptr<BaseProperty> baseProperty(std::move(property));

		AddProperty(name, std::move(baseProperty));

		return *this;
	}

	template <typename GetterSignature, typename SetterSignature>
	Class& DeclProperty(const std::string& name, GetterSignature getter, SetterSignature setter)
	{
		static_assert(std::is_same<ExtractValueType<GetterSignature>::type, ExtractValueType<SetterSignature>::type>::value, "Setter ang getter types mismatch!");

		auto property = std::make_unique<IndirectProperty<T, typename ExtractValueType<GetterSignature>::type, GetterSignature, SetterSignature>>(name, getter, setter);
		std::unique_ptr<BaseProperty> baseProperty(std::move(property));

		AddProperty(name, std::move(baseProperty));

		return *this;
	}
};

} // namespace rttr
