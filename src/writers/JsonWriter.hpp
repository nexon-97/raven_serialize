#pragma once
#include "rttr/Type.hpp"
#include <ostream>

class JsonWriter
{
public:
	explicit RAVEN_SER_API JsonWriter(std::ostream& stream, const bool writeClassNames = false);

	template <typename T>
	void Write(const T& value)
	{
		Write(rttr::Reflect<T>(), &value);
	}

	void RAVEN_SER_API Write(const rttr::Type& type, const void* value);

private:
	void PrintPadding();

private:
	std::ostream& m_stream;
	int m_padding = 0;
	bool m_writeClassNames = false;
};
