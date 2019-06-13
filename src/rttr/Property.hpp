#pragma once
#include "rttr/Aliases.hpp"
#include "rttr/helper/TypeTraitsExtension.hpp"
#include "rttr/Type.hpp"
#include <unordered_map>
#include <string>
#include <typeindex>
#include <cassert>

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

//MutatorMethod<T, ValueType>
template <typename T, typename ValueType>
void ApplySignature(MutatorMethod<T, ValueType> signature, T* object, const ValueType& value)
{
	std::invoke(signature, object, value);
}

template <typename T, typename ValueType>
void ApplySignature(MutatorMethodByValue<T, ValueType> signature, T* object, const ValueType& value)
{
	std::invoke(signature, object, value);
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

template <typename SignatureType>
struct ExtractClassType {};

template <typename T, typename ValueType>
struct ExtractClassType<MemberSignature<T, ValueType>>
{
	typedef T type;
};

template <typename T, typename ValueType>
struct ExtractClassType<AccessorMethod<T, ValueType>>
{
	typedef T type;
};

template <typename T, typename ValueType>
struct ExtractClassType<AccessorMethodByValue<T, ValueType>>
{
	typedef T type;
};

template <typename T, typename ValueType>
struct ExtractClassType<MutatorMethod<T, ValueType>>
{
	typedef T type;
};

template <typename T, typename ValueType>
struct ExtractClassType<MutatorMethodByValue<T, ValueType>>
{
	typedef T type;
};

///////////////////////////////////////////////////////////////////////////////////////

class Property
{
public:
	Property(const char* name, const Type& type)
		: m_name(name)
		, m_type(type)
	{}

	virtual void GetValue(const void* object, void*& storage, bool& needRelease) = 0;
	virtual void GetMutatorContext(const void* object, void*& storage, bool& needRelease) = 0;
	virtual void CallMutator(void* object, void* value) = 0;

	const Type& GetType() const
	{
		return m_type;
	}

	const char* GetName() const
	{
		return m_name;
	}

private:
	const char* m_name = nullptr;
	const Type m_type;
};

////////////////////////////////////////////////////////////////////////////////////////////

template <typename ClassType, typename ValueType, typename SignatureType>
class MemberProperty
	: public Property
{
public:
	MemberProperty(const char* name, SignatureType signature)
		: Property(name, rttr::Reflect<ValueType>())
		, m_signature(signature)
	{}

	void SetValue(ClassType* object, ValueType* value)
	{
		ApplySignature(m_signature, object, *value);
	}

	const ValueType& GetValue(const ClassType* object) const
	{
		return AccessBySignature(m_signature, object);
	}

	void GetValue(const void* object, void*& i_storage, bool& needRelease) final
	{
		// There is no need to allocate any additional memory, just cast the member pointer to void* and return
		const void* valuePtr = &AccessBySignature(m_signature, static_cast<const ClassType*>(object));
		i_storage = const_cast<void*>(valuePtr);
		needRelease = false;
	}

	void GetMutatorContext(const void* object, void*& i_storage, bool& needRelease) final
	{
		const void* valuePtr = &AccessBySignature(m_signature, static_cast<const ClassType*>(object));
		i_storage = const_cast<void*>(valuePtr);
		needRelease = false;
	}

	void CallMutator(void* object, void* value) final
	{
		SetValue(static_cast<ClassType*>(object), static_cast<ValueType*>(value));
	}

private:
	SignatureType m_signature;
};

template <typename ClassType, typename ValueType, typename GetterSignature, typename SetterSignature>
class IndirectProperty
	: public Property
{
public:
	IndirectProperty(const char* name, GetterSignature getterSignature, SetterSignature setterSignature)
		: Property(name, rttr::Reflect<ValueType>())
		, m_getterSignature(getterSignature)
		, m_setterSignature(setterSignature)
	{}

	void SetValue(ClassType* object, ValueType* value)
	{
		ApplySignature(m_setterSignature, object, *value);
	}

	const ValueType& GetValue(const ClassType* object) const
	{
		return AccessBySignature(m_getterSignature, object);
	}

	void GetValue(const void* object, void*& i_storage, bool& needRelease) final
	{
		auto storage = new ValueType(AccessBySignature(m_getterSignature, static_cast<const ClassType*>(object)));
		i_storage = storage;
		needRelease = true;
	}

	void GetMutatorContext(const void* object, void*& i_storage, bool& needRelease) final
	{
		auto storage = new ValueType();
		i_storage = storage;
		needRelease = true;
	}

	void CallMutator(void* object, void* value) final
	{
		SetValue(static_cast<ClassType*>(object), static_cast<ValueType*>(value));
	}

private:
	GetterSignature m_getterSignature;
	SetterSignature m_setterSignature;
};

} // namespace rttr
