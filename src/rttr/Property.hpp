#pragma once
#include "rttr/Aliases.hpp"
#include "rttr/helper/TypeTraitsExtension.hpp"
#include <unordered_map>
#include <string>
#include <typeindex>
#include <cassert>
#include <any>

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
const ValueType& AccessBySignature(SignatureType signature, const T* object)
{
	static_assert(false, "Cannot access by signature!");
}

template <typename T, typename ValueType>
const ValueType& AccessBySignature(MemberSignature<T, ValueType> signature, const T* object)
{
	return object->*signature;
}

template <typename T, typename ValueType>
const ValueType& AccessBySignature(AccessorMethod<T, ValueType> signature, const T* object)
{
	return std::invoke(signature, object);
}

template <typename T, typename ValueType>
ValueType AccessBySignature(AccessorMethodByValue<T, ValueType> signature, const T* object)
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



///////////////////////////////////////////////////////////////////////////////////////

class BaseProperty
{
public:
	BaseProperty(const std::string& name)
		: m_name(name)
	{}

	virtual std::type_index GetValueTypeIndex() const = 0;
	virtual void* GetValue(void* object) = 0;

	virtual std::any GetValueSafe(const void* object) const = 0;

	virtual bool IsIntegral() const = 0;
	virtual bool IsArray() const = 0;
	virtual std::size_t GetArraySize() const = 0;
	virtual std::any GetArrayItem(void* object, const std::size_t idx) const = 0;

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

	const ValueType& GetValue(const ClassType* object) const
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

	bool IsIntegral() const final
	{
		return std::is_integral<ValueType>::value;
	}

	bool IsArray() const final
	{
		constexpr const bool k_isArray = std::is_array<ValueType>::value || is_std_vector<ValueType>::value || is_std_array<ValueType>::value;
		return k_isArray;
	}

	std::size_t GetArraySize() const final
	{
		assert(IsArray());
		return std::extent<ValueType>::value;
	}

	std::any GetArrayItem(void* object, const std::size_t idx) const final
	{
		assert(IsArray());
		const ValueType& valueRef = AccessBySignature(m_signature, static_cast<ClassType*>(object));
		return valueRef;
	}

	std::any GetValueSafe(const void* object) const final
	{
		return GetValue(static_cast<const ClassType*>(object));
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

	const ValueType& GetValue(const ClassType* object) const
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

	bool IsIntegral() const final
	{
		return std::is_integral<ValueType>::value;
	}

	bool IsArray() const final
	{
		constexpr const bool k_isArray = std::is_array<ValueType>::value || is_std_vector<ValueType>::value || is_std_array<ValueType>::value;
		return k_isArray;
	}

	std::size_t GetArraySize() const final
	{
		assert(IsArray());
		return std::extent<ValueType>::value;
	}

	std::any GetArrayItem(void* object, const std::size_t idx) const final
	{
		assert(IsArray());
		const ValueType& valueRef = AccessBySignature(m_getterSignature, static_cast<ClassType*>(object));
		return valueRef;
	}

	std::any GetValueSafe(const void* object) const final
	{
		return GetValue(static_cast<const ClassType*>(object));
	}

private:
	GetterSignature m_getterSignature;
	SetterSignature m_setterSignature;
};

} // namespace rttr
