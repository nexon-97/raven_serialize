#pragma once
#include <cstddef>

namespace rttr
{

struct ArrayParams
{
	std::size_t arrayRank = 0U;
	std::size_t* arrayExtents = nullptr;
	Type arrayType;
};

template <typename T, std::size_t ...Is>
void FillArrayExtent(std::size_t* arrayExtents, std::index_sequence<Is...>)
{
	((arrayExtents[std::integral_constant<std::size_t, Is>{}] = std::extent_v<T, Is>), ...);
}

template <typename T, typename Cond = void>
struct ArrayTraitsResolver
{
	void operator()(ArrayParams& params) {}
};

template <typename T>
struct ArrayTraitsResolver<T, std::enable_if_t<std::is_array_v<T>>>
{
	void operator()(ArrayParams& params)
	{
		params.arrayRank = std::rank_v<T>;
		params.arrayExtents = new std::size_t[params.arrayRank];
		FillArrayExtent<T>(params.arrayExtents, std::make_index_sequence<std::rank_v<T>>());
		params.arrayType = Reflect<std::remove_all_extents_t<T>>();
	}
};

}
