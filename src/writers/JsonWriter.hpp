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
		WriteObject(rttr::Reflect<T>(), &value);
		m_jsonRoot = std::move(*m_jsonStack.top());
		m_jsonStack.pop();

		DoWrite();
	}

	void RAVEN_SER_API WriteObject(const rttr::Type& type, const void* value) final;
	void RAVEN_SER_API WriteArray(const rttr::Type& itemType, const void* arrayStartPtr, const std::size_t size) final;
	void RAVEN_SER_API WriteCustomObject(CustomObjectWriterFunc* objectWritersPtr, const std::size_t writersCount) final;

	void RAVEN_SER_API AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver) final;

private:
	void RAVEN_SER_API DoWrite();
	void GenerateSerializationContextValues();
	std::string WStringToUtf8(const wchar_t* _literal);
	void CreateSerializationContext();
	Json::Value& CreateJsonObject(const Json::ValueType valueType);

private:
	std::ostream& m_stream;
	std::unordered_map<std::type_index, rttr::PointerTypeResolver*> m_customPointerTypeResolvers;

	// Json stack is used to hide serialization stack operations on json values from end user
	// [USAGE] Create json values using private CreateJsonObject method,
	// then convert returned reference to a pointer and store inside json stack,
	// before calling the any of the write function recursively.
	// Do not pop the value, that you have pushed, in the same recursion level. It is the responsibility of the calling method.
	std::stack<Json::Value*> m_jsonStack;

	Json::Value m_jsonRoot;
	std::unique_ptr<rs::detail::SerializationContext> m_context;
	std::vector<Json::Value> m_serializedObjects;
	std::vector<std::unique_ptr<Json::Value>> m_contextObjects;
	const bool m_prettyPrint;
	bool m_isUsingPointerContext = false;
};
