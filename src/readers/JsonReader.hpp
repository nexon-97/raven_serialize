#pragma once
#include "readers/BaseReader.hpp"
#include "rttr/Type.hpp"
#include "rttr/CustomTypeResolver.hpp"
#include "actions/IReaderAction.hpp"
#include "SerializationContext.hpp"
#include "ContextPath.hpp"

#include <istream>
#include <unordered_map>
#include <json/json.h>

namespace rttr
{
template <typename T>
Type Reflect();
}

namespace rs
{

class JsonReader
	: public BaseReader
{
public:
	explicit RAVEN_SERIALIZE_API JsonReader(std::istream& stream);
	RAVEN_SERIALIZE_API ~JsonReader() = default;

	template <typename T>
	void Read(T& value)
	{
		BaseReader::Read(rttr::Reflect<T>(), &value);
	}

	bool RAVEN_SERIALIZE_API IsOk() const final;

protected:
	void DoRead(const rttr::Type& type, void* value) final;
	bool CheckSourceHasObjectsList() final;

private:
	ReadResult ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal);

	void ReadContextObject(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	Json::Value const* FindContextJsonObject(const Json::Value& jsonRoot, const uint64_t id) const;

	ReadResult ReadProxy(rttr::TypeProxyData* proxyTypeData, void* value, const Json::Value& jsonVal);
	ReadResult ReadObjectProperties(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount);
	ReadResult ReadCollection(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount);
	ReadResult ReadObjectBases(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	ReadResult ReadPointer(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	ReadResult ReadArray(const rttr::Type& type, void* value, const Json::Value& jsonVal);

	rttr::Type DeduceType(const Json::Value& jsonVal) const;
	void SortActions();

private:
	Json::Value m_jsonRoot;
	bool m_isOk = false;
	bool m_isReading = false;
};

} // namespace rs
