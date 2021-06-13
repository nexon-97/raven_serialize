#include "raven_serialize_export.h"
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
	success &= other.success;
	allEntitiesResolved &= other.allEntitiesResolved;
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
