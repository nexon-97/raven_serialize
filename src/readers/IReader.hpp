#pragma once
#include "rttr/Type.hpp"
#include "rttr/CustomTypeResolver.hpp"
#include "readers/ReadResult.hpp"

#include <unordered_map>

namespace rs
{

class IReader
{
public:
	virtual ~IReader() = default;

	// Register custom type resolver
	virtual void AddCustomTypeResolver(const rttr::Type& type, rttr::CustomTypeResolver* resolver) = 0;
	// Read is called by library users, it must handle some BeginRead and EndRead logic
	virtual void Read(const rttr::Type& type, void* value) = 0;
	// Notifies if the reader object is ready to perform reading
	virtual bool IsOk() const = 0;

protected:
	// Actually reads data, assuming reading context is valid
	virtual void DoRead(const rttr::Type& type, void* value) = 0;
};

} // namespace rs
