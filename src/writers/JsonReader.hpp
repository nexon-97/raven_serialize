#pragma once
#include "rttr/Type.hpp"

#include <istream>
#include <json/json.h>

class JsonReader
{
public:
	explicit RAVEN_SER_API JsonReader(std::istream& stream);

	template <typename T>
	void Read(T& value)
	{
		Read(rttr::Reflect<T>(), &value, m_jsonRoot);
	}

	void RAVEN_SER_API Read(const rttr::Type& type, void* value, const Json::Value& jsonVal);

private:
	std::istream& m_stream;
	Json::Value m_jsonRoot;
};
