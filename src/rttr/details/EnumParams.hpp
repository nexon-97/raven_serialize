#pragma once
#include "rttr/Type.hpp"

namespace rttr
{

struct EnumParams
{
	Type underlyingType;
};

template <typename T, typename Cond = void>
struct EnumTraitsResolver
{
	void operator()(EnumParams& params) {}
};

template <typename T>
struct EnumTraitsResolver<T, std::enable_if_t<std::is_enum_v<T>>>
{
	void operator()(EnumParams& params)
	{
		params.underlyingType = Reflect<typename std::underlying_type<T>::type>();
	}
};

}
