#pragma once
#include "rttr/Aliases.hpp"
#include <vector>
#include <array>
#include <string>
#include <memory>

namespace rttr
{

template <typename>
struct is_string : std::false_type {};

template <>
struct is_string<std::string> : std::true_type {};

template <>
struct is_string<std::wstring> : std::true_type {};

template <>
struct is_string<const char*> : std::true_type {};

template <typename>
struct is_std_vector : std::false_type {};

template <typename T, typename A>
struct is_std_vector<std::vector<T, A>> : std::true_type {};

template <typename T>
struct std_vector_type
{
	typedef nullptr_t type;
};

template <typename T, typename A>
struct std_vector_type<std::vector<T, A>>
{
	typedef T type;
};

template <typename>
struct is_std_array : std::false_type {};

template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template <typename>
struct is_smart_ptr : std::false_type {};

template <typename T>
struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};

template <typename T>
struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};

template <typename>
struct smart_ptr_type
{
	typedef nullptr_t type;
};

template <typename T>
struct smart_ptr_type<std::shared_ptr<T>>
{
	typedef std::add_pointer_t<T> type;
};

template <typename T>
struct smart_ptr_type<std::unique_ptr<T>>
{
	typedef std::add_pointer_t<T> type;
};

template <typename>
struct smart_ptr_type_name_resolver {};

template <typename T>
struct smart_ptr_type_name_resolver<std::shared_ptr<T>>
{
	const char* operator()()
	{
		return "shared_ptr";
	}
};

template <typename T>
struct smart_ptr_type_name_resolver<std::unique_ptr<T>>
{
	const char* operator()()
	{
		return "unique_ptr";
	}
};

template <typename>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template <typename>
struct is_unique_ptr : std::false_type {};

template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

///////////////////////////////////////////////////////////////////////////////////////

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
const ValueType& AccessBySignature(AccessorMethodByValue<T, ValueType> signature, const T* object)
{
	return std::invoke(signature, object);
}

template <typename T, typename ValueType>
ValueType AccessBySignature(AccessorMethodByValueNonConst<T, ValueType> signature, const T* object)
{
	return std::invoke(signature, object);
}

template <typename T, typename ValueType>
const ValueType& AccessBySignature(AccessorMethodNonConst<T, ValueType> signature, const T* object)
{
	return std::invoke(signature, const_cast<T*>(object));
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

}
