#pragma once
#include "rttr/Type.hpp"
#include "rttr/PointerTypeResolver.hpp"

class IWriter
{
public:
	virtual void AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver) = 0;

	// Used to write single data object, given its meta-type and pointer to object
	virtual void WriteObject(const rttr::Type& type, const void* value) = 0;
	// Used to write array of data objects, given single object meta-type, pointer to array start, and array size
	virtual void WriteArray(const rttr::Type& itemType, const void* arrayStartPtr, const std::size_t size) = 0;
	// Used to write custom object using array ob writer functions, that perform operations on writer
	using CustomObjectWriterFunc = std::function<void(IWriter*)>;
	virtual void WriteCustomObject(CustomObjectWriterFunc* objectWritersPtr, const std::size_t writersCount) = 0;
};
