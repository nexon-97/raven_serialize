#pragma once
#include "rttr/Type.hpp"

namespace rttr
{

struct PointerParams
{
	Type pointedType;
};

template <typename T, typename Cond = void>
struct PointerTraitsResolver
{
	void operator()(PointerParams& params) {}
};

template <typename T>
struct PointerTraitsResolver<T, std::enable_if_t<std::is_pointer_v<T>>>
{
	void operator()(PointerParams& params)
	{
		params.pointedType = Reflect<typename std::pointer_traits<T>::element_type>();
	}
};

}
