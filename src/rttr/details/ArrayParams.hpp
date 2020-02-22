#pragma once
#include <cstddef>

namespace rttr
{

struct ArrayParams
{
	std::size_t arrayRank = 0U;
	std::size_t* arrayExtents = nullptr;
	bool isStdVector : 1;
	bool isStdArray : 1;
	bool isSimpleArray : 1;
};

}
