#pragma once
#include "rttr/Property.hpp"
#include <vector>

namespace rttr
{

struct ObjectClassParams
{
	std::vector<std::unique_ptr<Property>> properties;
};

}
