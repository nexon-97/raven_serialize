#pragma once

namespace rs
{

struct RAVEN_SERIALIZE_API ReadResult
{
	bool success = true;
	bool allEntitiesResolved = true;

	ReadResult(bool success, bool allEntitiesResolved);

	bool Succeeded() const;
	void Merge(const ReadResult& other);

	static const ReadResult& OKResult();
	static const ReadResult& GenericFailResult();
};

}
