#pragma once
#include "rttr/PointerTypeResolver.hpp"
#include <ostream>
#include <memory>

class JsonWriter
{
public:
	explicit RAVEN_SER_API JsonWriter(std::ostream& stream);

	template <typename T>
	void Write(const T& value)
	{
		Write(rttr::Reflect<T>(), &value);
	}

	void RAVEN_SER_API Write(const rttr::Type& type, const void* value);

	void RAVEN_SER_API AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver);

private:
	void PrintPadding();

private:
	std::ostream& m_stream;
	std::unordered_map<std::type_index, rttr::PointerTypeResolver*> m_customPointerTypeResolvers;
	int m_padding = 0;
};
