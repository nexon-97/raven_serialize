#pragma once
#include "rttr/GenericTypeSpecializer.hpp"
#include "rttr/Property.hpp"

#include <utility>

namespace rttr
{

template <typename T, typename U>
struct GenericTypeSpecializer<std::pair<T, U>>
{
	using PairT = typename std::pair<T, U>;

	GenericTypeSpecializer(type_data& typeData)
	{
		auto keyProperty = CreateMemberProperty("key", &PairT::first);
		auto valProperty = CreateMemberProperty("val", &PairT::second);

		typeData.typeParams.object->properties.push_back(std::move(keyProperty));
		typeData.typeParams.object->properties.push_back(std::move(valProperty));
	}
};

}
