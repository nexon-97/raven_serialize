#pragma once
#include "rttr/Aliases.hpp"
#include <unordered_map>
#include <string>

namespace rttr
{

template <typename SignatureType, typename T, typename ValueType>
void ApplySignature(SignatureType signature, T* object, const ValueType& value)
{
	static_assert(false, "Cannot apply signature!");
}

template <typename T, typename ValueType>
void ApplySignature(MemberSignature<T, ValueType> signature, T* object, const ValueType& value)
{
	object->*signature = value;
}

///////////////////////////////////////////////////////////////////////////////////////

template <typename SignatureType, typename T, typename ValueType>
const ValueType& AccessBySignature(SignatureType signature, T* object)
{
	static_assert(false, "Cannot access by signature!");
}

template <typename T, typename ValueType>
const ValueType& AccessBySignature(MemberSignature<T, ValueType> signature, T* object)
{
	return object->*signature;
}

///////////////////////////////////////////////////////////////////////////////////////

template <typename SignatureType>
struct ExtractValueType {};

template <typename T, typename ValueType>
struct ExtractValueType<MemberSignature<T, ValueType>>
{
	typedef ValueType type;
};

///////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class BaseProperty
{
public:
	BaseProperty(const std::string& name)
		: m_name(name)
	{}

private:
	std::string m_name;
};

template <typename T, typename ValueType, typename SignatureType>
class Property
	: public BaseProperty<T>
{
public:
	Property(const std::string& name, SignatureType signature)
		: BaseProperty(name)
		, m_signature(signature)
	{}

	void SetValue(T* object, const ValueType& value)
	{
		ApplySignature(m_signature, object, value);
	}

	const ValueType& GetValue(T* object) const
	{
		return AccessBySignature(m_signature, object);
	}

private:
	SignatureType m_signature;
};

} // namespace rttr
