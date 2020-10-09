#pragma once
#include "writers/IWriter.hpp"
#include "SerializationContext.hpp"

#include <ostream>
#include <memory>
#include <json/json.h>

namespace rs
{

class JsonWriter
	: public IWriter
{
public:
	//void RAVEN_SERIALIZE_API WriteObject(const rttr::Type& type, const void* value) final;
	//void RAVEN_SERIALIZE_API WriteArray(const rttr::Type& itemType, const void* arrayStartPtr, const std::size_t size) final;
	//void RAVEN_SERIALIZE_API WriteCustomObject(CustomObjectWriterFunc* objectWritersPtr, const std::size_t writersCount) final;

private:
	//void RAVEN_SERIALIZE_API PreWrite();
	//void RAVEN_SERIALIZE_API DoWrite();
	//void GenerateSerializationContextValues();
	//std::string WStringToUtf8(const wchar_t* _literal);
	//void CreateSerializationContext();
	//Json::Value& CreateJsonObject(const Json::ValueType valueType);

protected:
	// Json stack is used to hide serialization stack operations on json values from end user
	// [USAGE] Create json values using private CreateJsonObject method,
	// then convert returned reference to a pointer and store inside json stack,
	// before calling the any of the write function recursively.
	// Do not pop the value, that you have pushed, in the same recursion level. It is the responsibility of the calling method.
	//std::stack<Json::Value*> m_jsonStack;

	//Json::Value m_jsonRoot;
	std::unique_ptr<rs::detail::SerializationContext> m_context;
	//std::vector<Json::Value> m_serializedObjects;
	//std::vector<std::unique_ptr<Json::Value>> m_contextObjects;
	//const bool m_prettyPrint;
	//bool m_isUsingPointerContext = false;
};

} // namespace rs
