#pragma once
#include "writers/IWriter.hpp"
#include "rttr/PointerTypeResolver.hpp"

#include <ostream>
#include <memory>
#include <json/json.h>

class JsonWriter
	: public IWriter
{
public:
	explicit RAVEN_SER_API JsonWriter(std::ostream& stream, const bool prettyPrint = true);

	template <typename T>
	void Write(const T& value)
	{
		m_jsonStack.push(&m_jsonRoot);
		Write(rttr::Reflect<T>(), &value);

		Json::StreamWriterBuilder writerBuilder;
		writerBuilder["indentation"] = "\t";
		Json::StreamWriter* jsonWriter = writerBuilder.newStreamWriter();
		jsonWriter->write(m_jsonRoot, &m_stream);
	}

	void RAVEN_SER_API Write(const rttr::Type& type, const void* value) final;
	void RAVEN_SER_API AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver) final;

private:
	std::string WStringToUtf8(const wchar_t* _literal);

private:
	std::ostream& m_stream;
	std::unordered_map<std::type_index, rttr::PointerTypeResolver*> m_customPointerTypeResolvers;
	Json::Value m_jsonRoot;
	std::stack<Json::Value*> m_jsonStack;
	const bool m_prettyPrint;
};
