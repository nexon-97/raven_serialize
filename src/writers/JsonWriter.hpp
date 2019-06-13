#pragma once
#include "rttr/Type.hpp"
#include <ostream>

class JsonWriter
{
public:
	explicit JsonWriter(std::ostream& stream);

	template <typename T>
	void Write(const T& value)
	{
		Write(rttr::Reflect<T>(), &value);
	}

	void Write(const rttr::Type& type, const void* value);

private:
	void PrintPadding();

private:
	std::ostream& m_stream;
	int m_padding = 0;
};
