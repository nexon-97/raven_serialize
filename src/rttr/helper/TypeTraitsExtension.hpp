#pragma once
#include "rttr/Aliases.hpp"
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <cstring>
#include <functional>

namespace rttr
{

template <typename>
struct ApplySignatureT {};

template <typename T, typename ValueType>
struct ApplySignatureT<MemberSignature<T, ValueType>>
{
	using SignatureT = MemberSignature<T, ValueType>;
	SignatureT m_signature;

	ApplySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	void operator()(T* object, const ValueType& value)
	{
		object->*m_signature = value;
	}
};

template <typename T, typename ValueType, std::size_t N>
struct ApplySignatureT<MemberSignature<T, ValueType[N]>>
{
	using SignatureT = MemberSignature<T, ValueType[N]>;
	SignatureT m_signature;

	ApplySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	void operator()(T* object, const ValueType* value)
	{
		void* memberPtr = object->*m_signature;
		std::memcpy(memberPtr, value, sizeof(ValueType) * N);
	}
};

template <typename T, typename ValueType>
struct ApplySignatureT<MutatorMethod<T, ValueType>>
{
	using SignatureT = MutatorMethod<T, ValueType>;
	SignatureT m_signature;

	ApplySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	void operator()(T* object, const ValueType& value)
	{
		std::invoke(m_signature, object, value);
	}
};

template <typename T, typename ValueType>
struct ApplySignatureT<MutatorMethodByValue<T, ValueType>>
{
	using SignatureT = MutatorMethodByValue<T, ValueType>;
	SignatureT m_signature;

	ApplySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	void operator()(T* object, ValueType value)
	{
		std::invoke(m_signature, object, value);
	}
};

///////////////////////////////////////////////////////////////////////////////////////

template <typename>
struct AccessBySignatureT {};

template <typename T, typename ValueType>
struct AccessBySignatureT<MemberSignature<T, ValueType>>
{
	using SignatureT = MemberSignature<T, ValueType>;
	SignatureT m_signature;

	AccessBySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	ValueType& operator()(const T* object)
	{
		T* removedConstObject = const_cast<T*>(object);
		return removedConstObject->*m_signature;
	}
};

template <typename T, typename ValueType>
struct AccessBySignatureT<AccessorMethod<T, ValueType>>
{
	using SignatureT = AccessorMethod<T, ValueType>;
	SignatureT m_signature;

	AccessBySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	const ValueType& operator()(const T* object)
	{
		return std::invoke(m_signature, object);
	}
};

template <typename T, typename ValueType>
struct AccessBySignatureT<AccessorMethodByValue<T, ValueType>>
{
	using SignatureT = AccessorMethodByValue<T, ValueType>;
	SignatureT m_signature;

	AccessBySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	ValueType operator()(const T* object)
	{
		return std::invoke(m_signature, object);
	}
};

template <typename T, typename ValueType>
struct AccessBySignatureT<AccessorMethodByValueNonConst<T, ValueType>>
{
	using SignatureT = AccessorMethodByValueNonConst<T, ValueType>;
	SignatureT m_signature;

	AccessBySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	ValueType operator()(const T* object)
	{
		T* removedConstObject = const_cast<T*>(object);
		return std::invoke(m_signature, removedConstObject);
	}
};

template <typename T, typename ValueType>
struct AccessBySignatureT<AccessorMethodNonConst<T, ValueType>>
{
	using SignatureT = AccessorMethodNonConst<T, ValueType>;
	SignatureT m_signature;

	AccessBySignatureT(SignatureT signature)
		: m_signature(signature)
	{}

	const ValueType& operator()(const T* object)
	{
		T* removedConstObject = const_cast<T*>(object);
		return std::invoke(m_signature, removedConstObject);
	}
};

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

template <typename T, typename ValueType>
struct ExtractValueType<AccessorMethodByValueNonConst<T, ValueType>>
{
	typedef ValueType type;
};

template <typename T, typename ValueType>
struct ExtractValueType<AccessorMethodNonConst<T, ValueType>>
{
	typedef ValueType type;
};

///////////////////////////////////////////////////////////////////////////////////////

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
struct ExtractClassType<AccessorMethodByValueNonConst<T, ValueType>>
{
	typedef T type;
};

template <typename T, typename ValueType>
struct ExtractClassType<AccessorMethodNonConst<T, ValueType>>
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

template <typename T>
struct IsMemberFuncPrototype : std::false_type {};

template <typename RetT, typename T, typename ...Args>
struct IsMemberFuncPrototype<RetT(T::*)(Args...)> : std::true_type {};

template <typename RetT, typename T, typename ...Args>
struct IsMemberFuncPrototype<RetT(T::*)(Args...) const> : std::true_type {};

template <typename T>
struct IsClassMemberPointer : std::false_type {};

template <typename ValueT, typename T>
struct IsClassMemberPointer<ValueT T::*> : std::true_type {};

}
