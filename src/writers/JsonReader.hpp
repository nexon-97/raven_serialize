#pragma once
#include "rttr/Type.hpp"
#include "rttr/PointerTypeResolver.hpp"

#include <istream>
#include <unordered_map>
#include <json/json.h>

class JsonReader
{
public:
	explicit RAVEN_SER_API JsonReader(std::istream& stream);

	template <typename T>
	void Read(T& value)
	{
		Read(rttr::Reflect<T>(), &value);
	}

	void RAVEN_SER_API Read(const rttr::Type& type, void* value);
	RAVEN_SER_API void* Read(const rttr::Type& type);

	bool RAVEN_SER_API IsOk() const;

	void RAVEN_SER_API AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver);

private:
	void RAVEN_SER_API ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	rttr::Type DeduceType(const Json::Value& jsonVal) const;

private:
	std::istream& m_stream;
	std::unordered_map<std::type_index, rttr::PointerTypeResolver*> m_customPointerTypeResolvers;
	Json::Value m_jsonRoot;
	bool m_isOk = false;
};
