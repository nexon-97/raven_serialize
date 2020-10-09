#pragma once
#include "readers/BaseReader.hpp"
#include "rttr/Type.hpp"
#include "actions/IReaderAction.hpp"
#include "SerializationContext.hpp"
#include "ContextPath.hpp"

#include <istream>
#include <unordered_map>
#include <json/json.h>

namespace rs
{

/*
* @brief Json reader implementation
*
* jsoncpp library is used to parse json content
*/
class JsonReader
	: public BaseReader
{
public:
	explicit RAVEN_SERIALIZE_API JsonReader(std::istream& stream);
	explicit RAVEN_SERIALIZE_API JsonReader(Json::Value&& jsonVal);
	explicit RAVEN_SERIALIZE_API JsonReader(const std::string& jsonContent);
	RAVEN_SERIALIZE_API ~JsonReader() = default;

	bool RAVEN_SERIALIZE_API IsOk() const final;

protected:
	void DoRead(const rttr::Type& type, void* value) final;
	bool CheckSourceHasObjectsList() final;

private:
	// Primary function to read any object type, will redirect to particular read method according to the type info
	ReadResult ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal);

	void ReadContextObject(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	Json::Value const* FindContextJsonObject(const Json::Value& jsonRoot, const uint64_t id) const;

	// Read object value from json, like it was proxy type, using proxy read converted (copy constructor from proxy to target type, etc)
	ReadResult ReadProxy(rttr::TypeProxyData* proxyTypeData, void* value, const Json::Value& jsonVal);
	// Read object named properties (like simple json object)
	ReadResult ReadObjectProperties(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount);
	// Read collection part of object
	// While object can contain collection traits, it can have other properties, that are serialized, except items
	// When collection is simple array of items, and stored as json array - just read it
	// If it's an object, we look for predefined object key name to find json array with items
	ReadResult ReadCollection(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount);
	// Read properties of base classes, they should be under predefined bases key
	ReadResult ReadObjectBases(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	ReadResult ReadPointer(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	// Read plain array type from json array
	ReadResult ReadArray(const rttr::Type& type, void* value, const Json::Value& jsonVal);

private:
	Json::Value m_jsonRoot;
	bool m_isOk = false;
};

} // namespace rs
