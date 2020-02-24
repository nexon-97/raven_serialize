#include "readers/JsonReader.hpp"
#include "rttr/Property.hpp"
#include "rttr/CustomTypeResolver.hpp"
#include "rttr/Manager.hpp"
#include "ptr/DefaultPointerFiller.hpp"
#include "ptr/PropertyPointerFiller.hpp"
#include "actions/CallObjectMutatorAction.hpp"
#include "actions/ResolvePointerAction.hpp"
#include "actions/CustomResolverAction.hpp"
#include "actions/CollectionInsertAction.hpp"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>

namespace
{

// Reserved serialization keywords
const char* k_typeId = "$type$";
const char* k_contextMasterObjectKey = "$master_obj$";
const char* k_contextObjectsKey = "$objects$";
const char* k_contextIdKey = "$id$";
const char* k_contextValKey = "$val$";
const char* k_collectionItemsKey = "$items$";

//////////////////////////////////////////////////////////////////////////////////
// Predefined Json type resolvers implementation

struct PredefinedJsonTypeResolver
{
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
		if (val.isObject() && val.isMember(k_contextIdKey) && val.isMember(k_contextValKey))
		{
			if (val[k_contextIdKey].asUInt64() == id)
			{
				return &val;
			}
		}
	}

	return nullptr;
}

void JsonReader::ReadContextObject(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	ReadResult objectReadResult = ReadImpl(type, value, jsonVal[k_contextValKey]);

	if (objectReadResult.success)
	{
		uint64_t objectId = (jsonVal[k_contextIdKey]).asUInt64();
		m_context->AddObject(objectId, type, value);
	}
	else
	{
		Log::LogMessage("Failed to read context object!");
	}
}

void JsonReader::DoRead(const rttr::Type& type, void* value)
{
	bool hasObjectsList = (m_jsonRoot.isObject() && m_jsonRoot.isMember(k_contextObjectsKey) && m_jsonRoot.isMember(k_contextMasterObjectKey));
	if (hasObjectsList)
	{
		// Parse objects list
		const Json::Value& contextObjectsVal = m_jsonRoot[k_contextObjectsKey];
		uint64_t masterObjectId = m_jsonRoot[k_contextMasterObjectKey].asUInt64();
		Json::Value const* masterObjectVal = nullptr;

		// Find master object json val
		for (const Json::Value& contextObjectVal : contextObjectsVal)
		{
			if (contextObjectVal.isMember(k_contextIdKey) && contextObjectVal.isMember(k_contextValKey) && contextObjectVal[k_contextIdKey].asUInt64() == masterObjectId)
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
						// Find object in context
						Json::Value const* contextJsonObjectPtr = FindContextJsonObject(contextObjectsVal, objectReference.first);
						if (nullptr != contextJsonObjectPtr)
						{
							rttr::Type pointedType = objectReference.second;
							if (pointedType.IsValid())
							{
								void* pointedValue = pointedType.Instantiate();
								ReadContextObject(pointedType, pointedValue, *contextJsonObjectPtr);
							}
							else
							{
								m_context->AddObject(objectReference.first, pointedType, nullptr);
							}
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
	bool hasObjectsList = (m_jsonRoot.isObject() && m_jsonRoot.isMember(k_contextObjectsKey) && m_jsonRoot.isMember(k_contextMasterObjectKey));
	return hasObjectsList;
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

		if (jsonVal.isMember(propertyName))
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
	else if (jsonVal.isObject() && jsonVal.isMember(k_collectionItemsKey))
	{
		// If this is a json object and has member with speicified name k_collectionItemsKey, use it as items container
		collectionItemsVal = &jsonVal[k_collectionItemsKey];
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
		rttr::TypeProxyData* proxyTypeData = rttr::Manager::GetRTTRManager().GetProxyType(type);
		if (nullptr != proxyTypeData)
		{
			Log::LogMessage("Type '%s' is read as proxy type '%s'", type.GetName(), proxyTypeData->proxyType.GetName());
			result = ReadProxy(proxyTypeData, value, jsonVal);
		}
		else
		{
			// Find custom type resolver for this type
			auto customResolverIt = m_customTypeResolvers.find(type.GetTypeIndex());
			if (customResolverIt != m_customTypeResolvers.end())
			{
				rttr::CustomTypeResolver* customTypeResolver = customResolverIt->second;

				// Serialized value type
				rttr::Type serializedValueType = DeduceType(jsonVal);
				void* serializedValue = nullptr;

				// Deserialize custom value from data source to pass to resolver
				if (serializedValueType.GetTypeIndex() != typeid(nullptr_t))
				{
					serializedValue = m_context->CreateTempVariable(serializedValueType);
					ReadImpl(serializedValueType, serializedValue, jsonVal);
				}

				auto resolverAction = std::make_unique<detail::CustomResolverAction>(0
					, type, value, serializedValueType, serializedValue, customTypeResolver);
				m_deferredCommandsList.push_back(std::move(resolverAction));
			}
			else
			{
				switch (type.GetTypeClass())
				{
					case rttr::TypeClass::Object:
					{
						result = ReadResult::OKResult();

						std::size_t propertiesCount = type.GetPropertiesCount();
						ReadResult propertiesReadResult = ReadObjectProperties(type, value, jsonVal, propertiesCount);
						result.Merge(propertiesReadResult);

						if (type.IsCollection())
						{
							Log::LogMessage("Type '%s' is collection", type.GetName());
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
						Log::LogMessage("Type is enum. Underlying type: '%s'", enumUnderlyingType.GetName());
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
							}
						}
						else
						{
							if (type.IsSignedIntegral())
							{
								int64_t intValue = jsonVal.asInt64();
								Log::LogMessage("Type is int. Value: %lld", intValue);

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
								Log::LogMessage("Type is uint. Value: %llu", intValue);

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
				}
			}
		}
	}

	return result;
}

bool JsonReader::IsOk() const
{
	return m_isOk;
}

rttr::Type JsonReader::DeduceType(const Json::Value& jsonVal) const
{
	/*switch (jsonVal.type())
	{
		case Json::ValueType::stringValue:
			return rttr::Reflect<const char*>();
		case Json::ValueType::booleanValue:
			return rttr::Reflect<bool>();
		case Json::ValueType::intValue:
			return rttr::Reflect<int64_t>();
		case Json::ValueType::uintValue:
			return rttr::Reflect<uint64_t>();
		case Json::ValueType::realValue:
			return rttr::Reflect<double>();
		case Json::ValueType::arrayValue:
			return rttr::Reflect<nullptr_t>();
		case Json::ValueType::objectValue:
		{
			if (jsonVal.isMember(k_typeId))
			{
				std::string typeId = jsonVal[k_typeId].asString();
				if (typeId == k_smartptrTypeKey)
				{
					return rttr::Reflect(jsonVal[k_sysTypeId].asCString());
				}
				else
				{
					rttr::Type reflectedType = rttr::Reflect(typeId.c_str());

					if (reflectedType.IsValid())
					{
						return reflectedType;
					}
					else
					{
						return rttr::Reflect<nullptr_t>();
					}
				}
			}
			else
			{
				return rttr::Reflect<nullptr_t>();
			}
		}
		case Json::ValueType::nullValue:
		default:
			return rttr::Reflect<nullptr_t>();
	}*/

	return rttr::Reflect<nullptr_t>();
}

} // namespace rs
