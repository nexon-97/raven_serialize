#pragma once
#include "readers/IReader.hpp"
#include "rttr/Type.hpp"
#include "rttr/CustomTypeResolver.hpp"
#include "actions/IReaderAction.hpp"
#include "SerializationContext.hpp"
#include "ContextPath.hpp"

#include <istream>
#include <unordered_map>
#include <json/json.h>

namespace rs
{

class JsonReader
	: public IReader
{
public:
	explicit RAVEN_SER_API JsonReader(std::istream& stream);

	template <typename T>
	void Read(T& value)
	{
		Read(rttr::Reflect<T>(), &value);
	}

	void RAVEN_SER_API Read(const rttr::Type& type, void* value) final;
	bool RAVEN_SER_API IsOk() const final;

	void RAVEN_SER_API AddCustomTypeResolver(const rttr::Type& type, rttr::CustomTypeResolver* resolver) final;

private:
	void RAVEN_SER_API ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal);
	rttr::Type DeduceType(const Json::Value& jsonVal) const;
	std::string GetObjectClassName(const Json::Value& jsonVal) const;
	void SortActions();

private:
	std::istream& m_stream;
	std::unordered_map<std::type_index, rttr::CustomTypeResolver*> m_customTypeResolvers;
	Json::Value m_jsonRoot;
	void* m_currentRootObject = nullptr;
	std::unique_ptr<rs::detail::SerializationContext> m_context;
	std::vector<std::unique_ptr<detail::IReaderAction>> m_actions;
	ContextPath m_contextPath;
	bool m_isOk = false;
};

} // namespace rs
