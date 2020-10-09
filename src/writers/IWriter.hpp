#pragma once
#include "rttr/Type.hpp"

namespace rttr
{
template <typename T>
Type Reflect();
}

namespace rs
{

class IWriter
{
public:
	virtual ~IWriter() = default;

	virtual bool Write(const rttr::Type& type, const void* value) = 0;

	template <typename T>
	bool TypedWrite(const T& value)
	{
		return Write(rttr::Reflect<T>(), &value);
	}
};

} // namespace rs
