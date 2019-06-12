#pragma once
#include "rttr/Aliases.hpp"
#include <unordered_map>
#include <string>
#include <typeindex>

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

template <typename T, typename ValueType>
const ValueType& AccessBySignature(AccessorMethod<T, ValueType> signature, T* object)
{
	return std::invoke(signature, object);
}

template <typename T, typename ValueType>
ValueType AccessBySignature(AccessorMethodByValue<T, ValueType> signature, T* object)
{
	return std::invoke(signature, object);
}

///////////////////////////////////////////////////////////////////////////////////////

template <typename SignatureType>
struct ExtractValueType {};

template <typename T, typename ValueType>
struct ExtractValueType<MemberSignature<T, ValueType>>
{
	typedef ValueType type;
};

template <typename T, typename ValueType>
struct ExtractValueType<AccessorMethod<T, ValueType>>
{
	typedef ValueType type;
};

template <typename T, typename ValueType>
struct ExtractValueType<AccessorMethodByValue<T, ValueType>>
{
	typedef ValueType type;
};

template <typename T, typename ValueType>
struct ExtractValueType<MutatorMethod<T, ValueType>>
{
	typedef ValueType type;
};

template <typename T, typename ValueType>
struct ExtractValueType<MutatorMethodByValue<T, ValueType>>
{
	typedef ValueType type;
};

///////////////////////////////////////////////////////////////////////////////////////

class BaseProperty
{
public:
	BaseProperty(const std::string& name)
		: m_name(name)
	{}

	virtual std::type_index GetValueTypeIndex() const = 0;
	virtual void* GetValue(void* object) = 0;

private:
	std::string m_name;
};

////////////////////////////////////////////////////////////////////////////////////////////

template <typename ClassType, typename ValueType, typename SignatureType>
class MemberProperty
	: public BaseProperty
{
public:
	MemberProperty(const std::string& name, SignatureType signature)
		: BaseProperty(name)
		, m_signature(signature)
	{}

	void SetValue(ClassType* object, const ValueType& value)
	{
		ApplySignature(m_signature, object, value);
	}

	const ValueType& GetValue(ClassType* object) const
	{
		return AccessBySignature(m_signature, object);
	}

	std::type_index GetValueTypeIndex() const final
	{
		return typeid(ValueType);
	}

	void* GetValue(void* object) final
	{
		static ValueType context;
		context = AccessBySignature(m_signature, static_cast<ClassType*>(object));

		return &context;
	}

private:
	SignatureType m_signature;
};

template <typename ClassType, typename ValueType, typename GetterSignature, typename SetterSignature>
class IndirectProperty
	: public BaseProperty
{
public:
	IndirectProperty(const std::string& name, GetterSignature getterSignature, SetterSignature setterSignature)
		: BaseProperty(name)
		, m_getterSignature(getterSignature)
		, m_setterSignature(setterSignature)
	{}

	void SetValue(ClassType* object, const ValueType& value)
	{
		ApplySignature(m_setterSignature, object, value);
	}

	const ValueType& GetValue(ClassType* object) const
	{
		return AccessBySignature(m_getterSignature, object);
	}

	std::type_index GetValueTypeIndex() const final
	{
		return typeid(ValueType);
	}

	void* GetValue(void* object) final
	{
		static ValueType context;
		context = AccessBySignature(m_getterSignature, static_cast<ClassType*>(object));

		return &context;
	}

private:
	GetterSignature m_getterSignature;
	SetterSignature m_setterSignature;
};

} // namespace rttr
