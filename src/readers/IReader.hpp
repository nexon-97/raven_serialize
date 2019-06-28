#pragma once
#include "rttr/Type.hpp"
#include "rttr/PointerTypeResolver.hpp"

namespace rs
{

class IReader
{
public:
	virtual ~IReader() = default;

	virtual void AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver) = 0;

	virtual void Read(const rttr::Type& type, void* value) = 0;
	virtual bool IsOk() const = 0;
};

} // namespace rs
