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

namespace rttr
{
template <typename T>
Type Reflect();
}

namespace rs
{

class JsonReader
	: public IReader
{
public:
	explicit RAVEN_SERIALIZE_API JsonReader(std::istream& stream);
	RAVEN_SERIALIZE_API ~JsonReader() = default;

	struct ReadResult
	{
		bool success = true;
		bool allEntitiesResolved = true;

		ReadResult(bool success, bool allEntitiesResolved)
			: success(success)
			, allEntitiesResolved(allEntitiesResolved)
		{}

		bool Succeeded() const
		{
			return success && allEntitiesResolved;
		}

		void Merge(const ReadResult& other)
		{
			if (other.success != success)
			{
				success = false;
			}

			if (other.allEntitiesResolved != allEntitiesResolved)
			{
				allEntitiesResolved = false;
			}
		}
	};

	template <typename T>
	void Read(T& value)
	{
		Read(rttr::Reflect<T>(), &value);
	}

	void RAVEN_SERIALIZE_API Read(const rttr::Type& type, void* value) final;
	bool RAVEN_SERIALIZE_API IsOk() const final;

	void RAVEN_SERIALIZE_API AddCustomTypeResolver(const rttr::Type& type, rttr::CustomTypeResolver* resolver) final;

	struct PredefinedJsonTypeResolver
	{
		virtual void Read(const rttr::Type& type, void* value, const Json::Value& jsonVal) = 0;
	};

private:
	ReadResult RAVEN_SERIALIZE_API ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal);

	ReadResult ReadProxy(rttr::TypeProxyData* proxyTypeData, void* value, const Json::Value& jsonVal);
	ReadResult ReadObjectProperties(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount);
	ReadResult ReadCollection(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount);
	ReadResult ReadPointer(const rttr::Type& type, void* value, const Json::Value& jsonVal);

	rttr::Type DeduceType(const Json::Value& jsonVal) const;
	std::string GetObjectClassName(const Json::Value& jsonVal) const;
	void SortActions();

private:
	std::istream& m_stream;
	std::unordered_map<std::type_index, rttr::CustomTypeResolver*> m_customTypeResolvers;
	std::unordered_map<std::type_index, std::unique_ptr<PredefinedJsonTypeResolver>> m_predefinedJsonTypeResolvers;
	Json::Value m_jsonRoot;
	void* m_currentRootObject = nullptr;
	std::unique_ptr<rs::detail::SerializationContext> m_context;
	std::vector<std::unique_ptr<detail::IReaderAction>> m_deferredCommandsList;
	std::vector<std::unique_ptr<rttr::CollectionInserterBase>> m_collectionInserters;
	ContextPath m_contextPath;
	bool m_isOk = false;
};

} // namespace rs
