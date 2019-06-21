#pragma once
#include "writers/IWriter.hpp"
#include "rttr/PointerTypeResolver.hpp"
#include <ostream>
#include <memory>

class JsonWriter
	: public IWriter
{
public:
	explicit RAVEN_SER_API JsonWriter(std::ostream& stream, const bool prettyPrint = true);

	template <typename T>
	void Write(const T& value)
	{
		Write(rttr::Reflect<T>(), &value);
	}

	void RAVEN_SER_API Write(const rttr::Type& type, const void* value) final;
	void RAVEN_SER_API AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver) final;

private:
	void PrintPadding();
	void WriteStringLiteral(const char* _literal);
	void WriteStringLiteral(const wchar_t* _literal);
	void WriteKeyValue(const char* key, const char* value);

private:
	std::ostream& m_stream;
	std::unordered_map<std::type_index, rttr::PointerTypeResolver*> m_customPointerTypeResolvers;
	int m_padding = 0;
	const bool m_prettyPrint;
};
