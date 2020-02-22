#pragma once

namespace rttr
{

struct ScalarParams
{
	bool isSigned : 1;
};

template <typename T, typename Cond = void>
struct ScalarTraitsResolver
{
	void operator()(ScalarParams& params) {}
};

template <typename T>
struct ScalarTraitsResolver<T, std::enable_if_t<std::is_integral_v<T>>>
{
	void operator()(ScalarParams& params)
	{
		params.isSigned = std::is_signed_v<T>;
	}
};

}
