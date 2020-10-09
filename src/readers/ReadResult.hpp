#pragma once

namespace rs
{

/*
* @brief Object read operation result
* success - defines if reader was able to get the object data correctly
* allEntitiesResolved - defines if reader was able to get all the sub-objects, or they should be treated separately
*/
struct RAVEN_SERIALIZE_API ReadResult
{
	bool success = true;
	bool allEntitiesResolved = true;

	ReadResult() = delete;
	explicit ReadResult(bool success, bool allEntitiesResolved);

	bool Succeeded() const;

	// Merged result succeeds, when the current and other results both succeeded
	void Merge(const ReadResult& other);

	static const ReadResult& OKResult();
	static const ReadResult& GenericFailResult();
};

}
