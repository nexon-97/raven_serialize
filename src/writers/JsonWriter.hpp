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
	JsonWriter() = default;
	~JsonWriter() = default;

	bool RAVEN_SERIALIZE_API Write(const rttr::Type& type, const void* value) override;
	RAVEN_SERIALIZE_API const Json::Value& GetJsonValue() const;

private:
	Json::Value WriteInternal(const rttr::Type& type, const void* value);
	void WriteObjectProperties(const rttr::Type& type, const void* value, Json::Value& jsonObject);
	Json::Value WriteProxy(rttr::TypeProxyData* proxyTypeData, const void* value);
	Json::Value WriteArray(const rttr::Type& type, const void* value);

protected:
	Json::Value m_jsonRoot;
	std::unique_ptr<rs::detail::SerializationContext> m_context;
};

} // namespace rs
