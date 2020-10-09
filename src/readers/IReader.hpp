#pragma once
#include "rttr/Type.hpp"
#include "readers/ReadResult.hpp"

#include <unordered_map>

namespace rttr
{
template <typename T>
Type Reflect();
}

namespace rs
{

class IReader
{
public:
	virtual ~IReader() = default;

	// Read is called by library users, it must handle some BeginRead and EndRead logic
	virtual void Read(const rttr::Type& type, void* value) = 0;
	// Notifies if the reader object is ready to perform reading
	virtual bool IsOk() const = 0;

	template <typename T>
	void TypedRead(T& value)
	{
		Read(rttr::Reflect<T>(), &value);
	}

protected:
	// Actually reads data, assuming reading context is valid
	virtual void DoRead(const rttr::Type& type, void* value) = 0;
};

} // namespace rs
