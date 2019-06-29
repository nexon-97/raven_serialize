#pragma once
#include "rttr/Type.hpp"
#include "rttr/CustomTypeResolver.hpp"

namespace rs
{

class IReader
{
public:
	virtual ~IReader() = default;

	virtual void AddCustomTypeResolver(const rttr::Type& type, rttr::CustomTypeResolver* resolver) = 0;

	virtual void Read(const rttr::Type& type, void* value) = 0;
	virtual bool IsOk() const = 0;
};

} // namespace rs
