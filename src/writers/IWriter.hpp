#pragma once
#include "rttr/Type.hpp"
#include "rttr/PointerTypeResolver.hpp"

class IWriter
{
public:
	virtual void Write(const rttr::Type& type, const void* value) = 0;
	virtual void AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver) = 0;
};
