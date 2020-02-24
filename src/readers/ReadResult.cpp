#include "readers/ReadResult.hpp"

namespace
{
const rs::ReadResult k_genericFailResult(false, true);
const rs::ReadResult k_okResult(true, true);
}

namespace rs
{

ReadResult::ReadResult(bool success, bool allEntitiesResolved)
	: success(success)
	, allEntitiesResolved(allEntitiesResolved)
{}

bool ReadResult::Succeeded() const
{
	return success && allEntitiesResolved;
}

void ReadResult::Merge(const ReadResult& other)
{
	if (other.success != success)
	{
		success = false;
	}

	if (other.allEntitiesResolved != allEntitiesResolved)
	{
		allEntitiesResolved = false;
	}
}

const ReadResult& ReadResult::OKResult()
{
	return k_okResult;
}

const ReadResult& ReadResult::GenericFailResult()
{
	return k_genericFailResult;
}

}
