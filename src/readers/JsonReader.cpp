#include "readers/JsonReader.hpp"
#include "rttr/Property.hpp"
#include "rttr/Manager.hpp"
#include "rs/SerializationKeywords.hpp"
#include "actions/CallObjectMutatorAction.hpp"
#include "actions/ResolvePointerAction.hpp"
#include "actions/CollectionInsertAction.hpp"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>

namespace
{

//////////////////////////////////////////////////////////////////////////////////
// Predefined Json type resolvers implementation

struct PredefinedJsonTypeResolver
{
	virtual ~PredefinedJsonTypeResolver() = default;
	virtual void Read(const rttr::Type& type, void* value, const Json::Value& jsonVal) = 0;
};

struct StdStringJsonTypeResolver
	: PredefinedJsonTypeResolver
{
	void Read(const rttr::Type& type, void* value, const Json::Value& jsonVal) override
	{
		if (jsonVal.isNull())
		{
			*static_cast<std::string*>(value) = std::string();
		}
		else if (jsonVal.isString())
		{
			*static_cast<std::string*>(value) = std::move(jsonVal.asString());
		}
	}
};

struct ConstCharStringJsonTypeResolver
	: PredefinedJsonTypeResolver
{
	void Read(const rttr::Type& type, void* value, const Json::Value& jsonVal) override
	{
		if (jsonVal.isNull())
		{
			char** strSerializedValue = reinterpret_cast<char**>(value);
			*strSerializedValue = nullptr;
		}
		else if (jsonVal.isString())
		{
			const char* strValue = jsonVal.asCString();
			char** strSerializedValue = reinterpret_cast<char**>(value);
			*strSerializedValue = const_cast<char*>(strValue);
		}
	}
};

std::unordered_map<std::type_index, std::unique_ptr<PredefinedJsonTypeResolver>> g_predefinedJsonTypeResolvers;

struct JsonTypeResolversInitContext
{
	JsonTypeResolversInitContext()
	{
		// Register predefined types resolvers
		g_predefinedJsonTypeResolvers.emplace(typeid(std::string), std::make_unique<StdStringJsonTypeResolver>());
		//m_predefinedJsonTypeResolvers.emplace(typeid(std::wstring), std::make_unique<StdStringJsonTypeResolver<std::wstring>>());
		g_predefinedJsonTypeResolvers.emplace(typeid(const char*), std::make_unique<ConstCharStringJsonTypeResolver>());
	}
};

const JsonTypeResolversInitContext typeResolversInitContext;

}

namespace rs
{

JsonReader::JsonReader(std::istream& stream)
{
	std::size_t startOffset = stream.tellg();

	stream.seekg(0, std::ios::end);
	std::size_t bufferSize = static_cast<std::size_t>(stream.tellg()) - startOffset;
	stream.seekg(startOffset, std::ios::beg);

	if (bufferSize > 0U)
	{
		char* buffer = new char[bufferSize];
		stream.read(buffer, bufferSize);

		Json::CharReaderBuilder builder;
		auto reader = builder.newCharReader();
		std::string errorStr;

		m_isOk = reader->parse(buffer, buffer + bufferSize, &m_jsonRoot, &errorStr);

		delete[] buffer;
	}
	else
	{
		m_isOk = false;
	}

	if (!m_isOk)
	{
		// Revert stream back to original offset
		stream.seekg(startOffset, std::ios::beg);
	}
}

void JsonReader::SortActions()
{
	using ActionPtr = std::unique_ptr<detail::IReaderAction>;
	auto predicate = [](const ActionPtr& lhs, const ActionPtr& rhs)
	{
		// First sort by action types
		if (lhs->GetActionType() != rhs->GetActionType())
		{
			return static_cast<int>(lhs->GetActionType()) < static_cast<int>(rhs->GetActionType());
		}

		// Then, if the same class, go the deeper operations
		return lhs->GetDepth() > rhs->GetDepth();
	};
	std::stable_sort(m_deferredCommandsList.begin(), m_deferredCommandsList.end(), predicate);
}

Json::Value const* JsonReader::FindContextJsonObject(const Json::Value& jsonRoot, const uint64_t id) const
{
	for (const Json::Value& val : jsonRoot)
	{
		if (val.isObject() && val.isMember(K_CONTEXT_OBJ_ID) && val.isMember(K_CONTEXT_OBJ_VAL))
		{
			if (val[K_CONTEXT_OBJ_ID].asUInt64() == id)
			{
				return &val;
			}
		}
	}

	return nullptr;
}

void JsonReader::ReadContextObject(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	ReadResult objectReadResult = ReadImpl(type, value, jsonVal[K_CONTEXT_OBJ_VAL]);

	if (objectReadResult.success)
	{
		uint64_t objectId = (jsonVal[K_CONTEXT_OBJ_ID]).asUInt64();
		m_context->AddObject(objectId, type, value);
	}
	else
	{
		Log::LogMessage("Failed to read context object!");
	}
}

void JsonReader::DoRead(const rttr::Type& type, void* value)
{
	bool hasObjectsList = (m_jsonRoot.isObject() && m_jsonRoot.isMember(K_CONTEXT_OBJECTS) && m_jsonRoot.isMember(K_MASTER_OBJ_ID));
	if (hasObjectsList)
	{
		// Parse objects list
		const Json::Value& contextObjectsVal = m_jsonRoot[K_CONTEXT_OBJECTS];
		uint64_t masterObjectId = m_jsonRoot[K_MASTER_OBJ_ID].asUInt64();
		Json::Value const* masterObjectVal = nullptr;

		// Find master object json val
		for (const Json::Value& contextObjectVal : contextObjectsVal)
		{
			if (contextObjectVal.isMember(K_CONTEXT_OBJ_ID) && contextObjectVal.isMember(K_CONTEXT_OBJ_VAL) && contextObjectVal[K_CONTEXT_OBJ_ID].asUInt64() == masterObjectId)
			{
				masterObjectVal = &contextObjectVal;
				break;
			}
		}

		if (nullptr != masterObjectVal)
		{
			// Parse master object
			ReadContextObject(type, value, *masterObjectVal);
			FilterReferencedObjectsList(m_referencedContextObjects);

			while (!m_referencedContextObjects.empty())
			{
				// Handle referenced context objects
				for (const auto& objectReference : m_referencedContextObjects)
				{
					bool objectAlreadyLoaded = !!m_context->GetObjectById(objectReference.first);
					if (!objectAlreadyLoaded)
					{
						bool contextObjectValid = false;

						// Find object in context
						Json::Value const* contextJsonObjectPtr = FindContextJsonObject(contextObjectsVal, objectReference.first);
						if (nullptr != contextJsonObjectPtr)
						{
							const Json::Value& contextJsonObject = *contextJsonObjectPtr;
							rttr::Type pointedType = objectReference.second;

							if (pointedType.IsValid())
							{
								// Check actual type of polymorphic type
								if (pointedType.IsPolymorphic() && contextJsonObject[K_CONTEXT_OBJ_VAL].isMember(K_TYPE_ID))
								{
									rttr::Type deducedType = rttr::Reflect(contextJsonObject[K_CONTEXT_OBJ_VAL][K_TYPE_ID].asCString());
									if (deducedType.IsValid() && deducedType.IsBaseClass(pointedType))
									{
										pointedType = deducedType;
									}
								}

								void* pointedValue = pointedType.Instantiate();
								if (nullptr != pointedValue)
								{
									ReadContextObject(pointedType, pointedValue, contextJsonObject);
									contextObjectValid = true;
								}
							}
						}

						if (!contextObjectValid)
						{
							m_context->AddObject(objectReference.first, objectReference.second, nullptr);
						}
					}
				}

				FilterReferencedObjectsList(m_referencedContextObjects);
			}
		}
		else
		{
			Log::LogMessage("Master object not found in the context objects list!");
		}
	}
	else
	{
		// We have single object, simply read it here
		ReadImpl(type, value, m_jsonRoot);
	}
}

bool JsonReader::CheckSourceHasObjectsList()
{
	bool hasObjectsList = (m_jsonRoot.isObject() && m_jsonRoot.isMember(K_CONTEXT_OBJECTS) && m_jsonRoot.isMember(K_MASTER_OBJ_ID));
	return hasObjectsList;
}

ReadResult JsonReader::ReadObjectBases(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	ReadResult result = ReadResult::OKResult();

	const auto& baseClassesInfo = type.GetBaseClasses();
	if (baseClassesInfo.second > 0U && jsonVal.isMember(K_BASES))
	{
		for (uint8_t i = 0U; i < baseClassesInfo.second; ++i)
		{
			bool baseClassResolved = false;
			rttr::Type baseClass = baseClassesInfo.first[i];

			for (const Json::Value& baseVal : jsonVal[K_BASES])
			{
				if (baseVal.isMember(K_BASE_ID) && baseVal[K_BASE_ID].asString() == baseClass.GetName())
				{
					ReadResult baseReadResult = ReadImpl(baseClass, value, baseVal);
					result.Merge(baseReadResult);

					baseClassResolved = true;
					break;
				}
			}

			if (!baseClassResolved)
			{
				Log::LogMessage("Base class '%s' not resolved!", baseClass.GetName());
			}
		}
	}

	return result;
}

ReadResult JsonReader::ReadObjectProperties(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount)
{
	ReadResult result = ReadResult::OKResult(); // If we have no properties, it's OK

	for (std::size_t i = 0U; i < propertiesCount; ++i)
	{
		rttr::Property* property = type.GetProperty(i);
		const rttr::Type& propertyType = property->GetType();
		const char* propertyName = property->GetName();

		Log::LogMessage("Reading property '%s::%s'", type.GetName(), propertyName);

		if (jsonVal.isObject() && jsonVal.isMember(propertyName))
		{
			const Json::Value& itemJsonVal = jsonVal[propertyName];

			// Decide to create temp variable or not
			void* propertyValuePtr = nullptr;
			bool needsTempVar = property->NeedsTempVariable();

			if (needsTempVar)
			{
				propertyValuePtr = m_context->CreateTempVariable(property->GetType());
			}
			else
			{
				propertyValuePtr = property->GetValueAddress(value);
			}

			ReadResult propertyReadResult = ReadImpl(propertyType, propertyValuePtr, itemJsonVal);

			if (propertyReadResult.Succeeded())
			{
				property->CallMutator(value, const_cast<void*>(propertyValuePtr));

				if (needsTempVar)
				{
					m_context->DestroyTempVariable(propertyValuePtr);
				}
			}
			else
			{
				if (!propertyReadResult.allEntitiesResolved)
				{
					// If not all property value entities are resolved, make use of deferred commands list
					auto callMutatorAction = std::make_unique<detail::CallObjectMutatorAction>(0, property, value, propertyValuePtr);
					m_deferredCommandsList.push_back(std::move(callMutatorAction));

					// Notify calling code that not all entities are resolved for this object
					result.allEntitiesResolved = false;
				}
			}
		}
		else
		{
			// If we couldn't find the property in json object, just log it, and produce no error
			Log::LogMessage("Property '%s' not found in json object!", propertyName);
		}
	}

	return result;
}

ReadResult JsonReader::ReadCollection(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount)
{
	ReadResult result = ReadResult::GenericFailResult();

	// Pick correct json value to get objects from
	Json::Value const* collectionItemsVal = nullptr;

	if (propertiesCount == 0 && jsonVal.isArray())
	{
		// If this type has no properties, and declared as array in json, use it as items container
		collectionItemsVal = &jsonVal;
	}
	else if (jsonVal.isObject() && jsonVal.isMember(K_COLLECTION_ITEMS))
	{
		// If this is a json object and has member with speicified name k_collectionItemsKey, use it as items container
		collectionItemsVal = &jsonVal[K_COLLECTION_ITEMS];
	}

	if (collectionItemsVal && collectionItemsVal->isArray())
	{
		// We have correct json object with items data, now get collection traits from meta type
		std::unique_ptr<rttr::CollectionInserterBase> inserter = type.CreateCollectionInserter(value);
		rttr::Type collectionItemType = type.GetCollectionItemType();

		if (inserter && collectionItemType.IsValid())
		{
			rttr::CollectionInserterBase* rawInserter = inserter.get();
			m_collectionInserters.push_back(std::move(inserter));

			// If we reach here, we have a valid collection
			result.success = true;

			int i = 0;
			for (const Json::Value& jsonItem : *collectionItemsVal)
			{
				Log::LogMessage("Reading collection item %d", i);

				void* collectionItem = m_context->CreateTempVariable(collectionItemType);
				ReadResult itemReadResult = ReadImpl(collectionItemType, collectionItem, jsonItem);

				if (itemReadResult.Succeeded())
				{
					// Item read successfully, so we can safely insert here and release item temp variable
					rawInserter->Insert(collectionItem);
					m_context->DestroyTempVariable(collectionItem);
				}
				else
				{
					Log::LogMessage("Collection item failed to be read!");

					if (!itemReadResult.allEntitiesResolved)
					{
						// Notify caller that not all entities are resolved, and we must now defer actions using commands list
						result.allEntitiesResolved = false;

						// Not all entities of collection item are resolved, put insert command to deferred commands list
						auto insertAction = std::make_unique<detail::CollectionInsertAction>(0, rawInserter, collectionItem);
						m_deferredCommandsList.push_back(std::move(insertAction));
					}
					
					// For other error cases, we just skip the item and don't add it to final collection
				}

				++i;
			}
		}
	}

	return result;
}

ReadResult JsonReader::ReadPointer(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	ReadResult result = ReadResult::GenericFailResult();

	if (jsonVal.isNull())
	{
		// Resolve null pointer
		rttr::AssignPointerValue(value, nullptr);
		result = ReadResult::OKResult();
	}
	else if (jsonVal.isUInt())
	{
		uint64_t objectId = jsonVal.asUInt64();
		m_referencedContextObjects.emplace_back(objectId, type.GetPointedType());

		result = ReadResult::OKResult();

		// If we have resolved pointer address right now, use it
		auto referencedObjectData = m_context->GetObjectById(objectId);
		if (nullptr != referencedObjectData)
		{
			rttr::AssignPointerValue(value, referencedObjectData->objectPtr);
		}
		else
		{
			// Pointer can't be resolved right now, so put resolve action into deferred commands list
			auto resolvePtrAction = std::make_unique<detail::ResolvePointerAction>(0, m_context.get(), value, objectId);
			m_deferredCommandsList.push_back(std::move(resolvePtrAction));

			result.allEntitiesResolved = false;
		}
	}

	return result;
}

ReadResult JsonReader::ReadProxy(rttr::TypeProxyData* proxyTypeData, void* value, const Json::Value& jsonVal)
{
	ReadResult result = ReadResult::GenericFailResult();

	if (proxyTypeData->readConverter)
	{
		// Create proxy object
		void* proxyObject = m_context->CreateTempVariable(proxyTypeData->proxyType);

		// Read proxy object
		result = ReadImpl(proxyTypeData->proxyType, proxyObject, jsonVal);

		// Create target object using proxy constructor
		proxyTypeData->readConverter->Convert(value, proxyObject);
	}
	else
	{
		Log::LogMessage("Type has proxy type, but no read converter defined!");
	}

	return result;
}

ReadResult JsonReader::ReadArray(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	ReadResult result = ReadResult::GenericFailResult();

	if (jsonVal.isArray())
	{
		result = ReadResult::OKResult();

		rttr::Type arrayType = type.GetArrayType();
		uint8_t* arrayBytePtr = static_cast<uint8_t*>(value);
		std::size_t itemSize = arrayType.GetSize();

		std::size_t totalSize = type.GetArrayExtent(0U);
		for (std::size_t i = 1U; i < type.GetArrayRank(); ++i)
		{
			totalSize *= type.GetArrayExtent(i);
		}

		std::size_t i = 0U;
		for (const Json::Value& arrayItemVal : jsonVal)
		{
			if (i >= totalSize)
			{
				Log::LogMessage("Actual json array doesn't fit in target array size!");
				break;
			}

			uint8_t* itemPtr = arrayBytePtr + itemSize * i;
			ReadResult itemResult = ReadImpl(arrayType, itemPtr, arrayItemVal);

			if (!itemResult.Succeeded())
			{
				if (!itemResult.allEntitiesResolved)
				{
					result.allEntitiesResolved = false;
				}
			}

			++i;
		}
	}

	return result;
}

ReadResult JsonReader::ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	assert(m_isOk);

	ReadResult result = ReadResult::GenericFailResult();

	// Find in predefined types list
	auto predefinedTypeIt = g_predefinedJsonTypeResolvers.find(type.GetTypeIndex());
	if (predefinedTypeIt != g_predefinedJsonTypeResolvers.end())
	{
		predefinedTypeIt->second->Read(type, value, jsonVal);
		result = ReadResult::OKResult(); // Check errors from predefined types
	}
	else
	{
		rs::SerializationMethod serializationMethod = type.GetSerializationMethod();

		switch (serializationMethod)
		{
			case rs::SerializationMethod::Proxy:
			{
				rttr::TypeProxyData* proxyTypeData = rttr::Manager::GetRTTRManager().GetProxyType(type);
				if (nullptr != proxyTypeData)
				{
					result = ReadProxy(proxyTypeData, value, jsonVal);
				}
			}
			break;
			case rs::SerializationMethod::Adapter:
			{
				SerializationAdapter* adapter = rttr::Manager::GetRTTRManager().GetSerializationAdapter(type);
				if (nullptr != adapter)
				{
					// Parse payload
					SerializationAdapter::DataChunk payload;
					payload.type = adapter->GetPayloadType();

					if (payload.type.IsValid() && jsonVal.isObject() && jsonVal.isMember(K_ADAPTER))
					{
						// If it's a json object with adapter member, treat it as payload
						payload.value = m_context->CreateTempVariable(payload.type);
						ReadResult payloadReadResult = ReadImpl(payload.type, payload.value, jsonVal[K_ADAPTER]);
					}

					// Perform adapter logic (payload can be empty)
					SerializationAdapter::AdapterReadOutput adapterOutput = adapter->ReadConvert(payload);

					// Read json value as converted type
					if (adapterOutput.convertedType.IsValid())
					{
						void* adapterValue = m_context->CreateTempVariable(adapterOutput.convertedType);
						result = ReadImpl(adapterOutput.convertedType, adapterValue, jsonVal);

						adapter->ReadFinalize(adapterValue, value, adapterOutput, payload);
					}
				}
			}
			break;
			default:
			{
				switch (type.GetTypeClass())
				{
					case rttr::TypeClass::Object:
					{
						result = ReadResult::OKResult();

						ReadResult basesReadResult = ReadObjectBases(type, value, jsonVal);
						result.Merge(basesReadResult);

						std::size_t propertiesCount = type.GetPropertiesCount();
						ReadResult propertiesReadResult = ReadObjectProperties(type, value, jsonVal, propertiesCount);
						result.Merge(propertiesReadResult);

						if (type.IsCollection())
						{
							ReadResult collectionReadResult = ReadCollection(type, value, jsonVal, propertiesCount);
							result.Merge(collectionReadResult);
						}
					}
					break;
					case rttr::TypeClass::Pointer:
					{
						result = ReadPointer(type, value, jsonVal);
					}
					break;
					case rttr::TypeClass::Enum:
					{
						rttr::Type enumUnderlyingType = type.GetEnumUnderlyingType();
						result = ReadImpl(enumUnderlyingType, value, jsonVal);
					}
					break;
					case rttr::TypeClass::Real:
					{
						double valueToAssign = 0.0;

						if (jsonVal.isNumeric())
						{
							valueToAssign = jsonVal.asDouble();
							result = ReadResult::OKResult();
						}
						else
						{
							result = ReadResult::GenericFailResult();
						}

						if (type.GetTypeIndex() == typeid(float))
						{
							*static_cast<float*>(value) = static_cast<float>(valueToAssign);
						}
						else
						{
							*static_cast<double*>(value) = valueToAssign;
						}
					}
					break;
					case rttr::TypeClass::Integral:
					{
						if (type.GetTypeIndex() == typeid(bool))
						{
							switch (jsonVal.type())
							{
							case Json::booleanValue:
							{
								*static_cast<bool*>(value) = jsonVal.asBool();
								result = ReadResult::OKResult();
							}
							break;
							case Json::intValue:
							case Json::uintValue:
							case Json::nullValue:
							{
								*static_cast<bool*>(value) = !!jsonVal.asUInt64();
								result = ReadResult::OKResult();
							}
							break;
							default:
							break;
							}
						}
						else
						{
							if (type.IsSignedIntegral())
							{
								int64_t intValue = jsonVal.asInt64();

								switch (type.GetSize())
								{
								case 1:
									*static_cast<int8_t*>(value) = static_cast<int8_t>(intValue);
									break;
								case 2:
									*static_cast<int16_t*>(value) = static_cast<int16_t>(intValue);
									break;
								case 4:
									*static_cast<int32_t*>(value) = static_cast<int32_t>(intValue);
									break;
								case 8:
								default:
									*static_cast<int64_t*>(value) = intValue;
									break;
								}

								result = ReadResult::OKResult();
							}
							else
							{
								uint64_t intValue = jsonVal.asUInt64();

								switch (type.GetSize())
								{
								case 1:
									*static_cast<uint8_t*>(value) = static_cast<uint8_t>(intValue);
									break;
								case 2:
									*static_cast<uint16_t*>(value) = static_cast<uint16_t>(intValue);
									break;
								case 4:
									*static_cast<uint32_t*>(value) = static_cast<uint32_t>(intValue);
									break;
								case 8:
								default:
									*static_cast<uint64_t*>(value) = intValue;
									break;
								}

								result = ReadResult::OKResult();
							}
						}
					}
					break;
					case rttr::TypeClass::Array:
					{
						result = ReadArray(type, value, jsonVal);
					}
					break;
					default:
						break;
				}
			}
			break;
		}
				
	}

	return result;
}

bool JsonReader::IsOk() const
{
	return m_isOk;
}

} // namespace rs
