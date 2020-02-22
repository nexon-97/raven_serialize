#pragma once
#include <type_traits>

namespace rttr
{

enum class TypeClass
{
	Invalid,

	Integral,
	Real,
	Array,
	Enum,
	Function,
	MemberFunction,
	MemberPointer,
	Pointer,
	Object,
};

template <typename T, typename Cond = void>
struct TypeClassResolver
{
	TypeClass operator()() { return TypeClass::Invalid; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_class_v<T>>>
{
	TypeClass operator()() { return TypeClass::Object; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_enum_v<T>>>
{
	TypeClass operator()() { return TypeClass::Enum; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_integral_v<T>>>
{
	TypeClass operator()() { return TypeClass::Integral; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_floating_point_v<T>>>
{
	TypeClass operator()() { return TypeClass::Real; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_function_v<T>>>
{
	TypeClass operator()() { return TypeClass::Function; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_array_v<T>>>
{
	TypeClass operator()() { return TypeClass::Array; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_pointer_v<T>>>
{
	TypeClass operator()() { return TypeClass::Pointer; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_member_function_pointer_v<T>>>
{
	TypeClass operator()() { return TypeClass::MemberFunction; }
};

template <typename T>
struct TypeClassResolver<T, std::enable_if_t<std::is_member_object_pointer_v<T>>>
{
	TypeClass operator()() { return TypeClass::MemberPointer; }
};

}
