#pragma once
#include "writers/IWriter.hpp"
#include "rttr/PointerTypeResolver.hpp"
#include "SerializationContext.hpp"

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
		m_jsonStack.pop();

		DoWrite();
	}

	void RAVEN_SER_API Write(const rttr::Type& type, const void* value) final;
	void RAVEN_SER_API AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver) final;

private:
	void RAVEN_SER_API DoWrite();
	void GenerateSerializationContextValues();
	std::string WStringToUtf8(const wchar_t* _literal);
	void CreateSerializationContext();

private:
	std::ostream& m_stream;
	std::unordered_map<std::type_index, rttr::PointerTypeResolver*> m_customPointerTypeResolvers;
	Json::Value m_jsonRoot;
	std::stack<Json::Value*> m_jsonStack;
	std::unique_ptr<rs::detail::SerializationContext> m_context;
	std::vector<Json::Value> m_serializedObjects;
	const bool m_prettyPrint;
	bool m_isUsingPointerContext = false;
};
