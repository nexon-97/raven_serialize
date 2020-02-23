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
const char* k_typeId = "$type$";
const char* k_contextTypeName = "@context@";
const char* k_contextObjectsKey = "$objects$";
const char* k_smartptrTypeKey = "@smartptr@";
const char* k_sysTypeId = "$sys_type$";
const char* k_smartptrPointedValueKey = "ptr_value";
const char* k_ptrMarkerKey = "$marker_id$";
const char* k_collectionItemsKey = "$items$";

const rs::JsonReader::ReadResult k_readSuccess(true, true);
const rs::JsonReader::ReadResult k_readFailedGeneric(false, true);

struct StdStringJsonTypeResolver
	: rs::JsonReader::PredefinedJsonTypeResolver
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
	: rs::JsonReader::PredefinedJsonTypeResolver
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

}

namespace rs
{

JsonReader::JsonReader(std::istream& stream)
	: m_stream(stream)
{
	std::size_t startOffset = stream.tellg();
	m_stream.seekg(0, std::ios::end);
	std::size_t bufferSize = static_cast<std::size_t>(m_stream.tellg()) - startOffset;
	m_stream.seekg(startOffset, std::ios::beg);

	char* buffer = new char[bufferSize];
	m_stream.read(buffer, bufferSize);

	Json::CharReaderBuilder builder;
	auto reader = builder.newCharReader();
	std::string errorStr;

	m_isOk = reader->parse(buffer, buffer + bufferSize, &m_jsonRoot, &errorStr);

	if (!m_isOk)
	{
		m_stream.seekg(startOffset, std::ios::beg);
	}

	delete[] buffer;

	// Register predefined types resolvers
	m_predefinedJsonTypeResolvers.emplace(typeid(std::string), std::make_unique<StdStringJsonTypeResolver>());
	//m_predefinedJsonTypeResolvers.emplace(typeid(std::wstring), std::make_unique<StdStringJsonTypeResolver<std::wstring>>());
	m_predefinedJsonTypeResolvers.emplace(typeid(const char*), std::make_unique<ConstCharStringJsonTypeResolver>());
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

void JsonReader::Read(const rttr::Type& type, void* value)
{
	//
	auto readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
	Log::LogMessage("=========\nJsonReader [0x%llX] started reading session\n=========", readerAsInt);

	std::string className = GetObjectClassName(m_jsonRoot);
	m_context = std::make_unique<rs::detail::SerializationContext>();

	if (className == k_contextTypeName)
	{
		// Parse context
		const Json::Value& contextObjects = m_jsonRoot[k_contextObjectsKey];
		std::size_t idx = 0U;
		for (const Json::Value& contextObject : contextObjects)
		{
			rttr::Type objectType = DeduceType(contextObject);

			if (objectType.IsValid())
			{
				if (idx == 0U)
				{
					m_currentRootObject = value;
				}
				else
				{
					m_currentRootObject = objectType.Instantiate();
				}

				//
				auto valueAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_currentRootObject));
				auto readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
				Log::LogMessage("JsonReader [0x%llX] started reading object: (%s, 0x%llX)", readerAsInt, objectType.GetName(), valueAsInt);

				ReadImpl(objectType, m_currentRootObject, contextObject);

				//
				valueAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_currentRootObject));
				readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
				Log::LogMessage("JsonReader [0x%llX] ended reading object: (%s, 0x%llX)", readerAsInt, objectType.GetName(), valueAsInt);

				// Check object marker
				if (contextObject.isObject() && contextObject.isMember(k_ptrMarkerKey))
				{
					std::size_t markerId = static_cast<std::size_t>(contextObject[k_ptrMarkerKey].asUInt());
					m_context->AddObject(markerId, objectType, m_currentRootObject);
				}

				m_currentRootObject = nullptr;
			}	

			++idx;
		}
	}
	else
	{
		m_currentRootObject = value;

		ReadImpl(type, m_currentRootObject, m_jsonRoot);

		m_currentRootObject = nullptr;
	}

	// Perform actions
	//SortActions();
	for (const auto& action : m_deferredCommandsList)
	{
		action->Perform();
	}

	m_context->ClearTempVariables();

	//
	readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
	Log::LogMessage("=========\nJsonReader [0x%llX] ended reading session\n=========", readerAsInt);
}

JsonReader::ReadResult JsonReader::ReadObjectProperties(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount)
{
	ReadResult result = k_readSuccess; // If we have no properties, it's OK

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
					auto callMutatorAction = std::make_unique<detail::CallObjectMutatorAction>(m_contextPath.GetSize(), property, value, propertyValuePtr);
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

JsonReader::ReadResult JsonReader::ReadCollection(const rttr::Type& type, void* value, const Json::Value& jsonVal, std::size_t propertiesCount)
{
	JsonReader::ReadResult result = k_readFailedGeneric;

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
						auto insertAction = std::make_unique<detail::CollectionInsertAction>(m_contextPath.GetSize(), rawInserter, collectionItem);
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

JsonReader::ReadResult JsonReader::ReadPointer(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	ReadResult result = k_readFailedGeneric;

	//if (jsonVal.isObject() && jsonVal.isMember(k_ptrMarkerKey))
	//{
	//	Log::LogMessage("Type is valid pointer");

	//	std::size_t markerId = static_cast<std::size_t>(jsonVal[k_ptrMarkerKey].asUInt());
	//	auto resolvePtrAction = std::make_unique<detail::ResolvePointerAction>(m_contextPath.GetSize(), m_context.get(), type, value, markerId);
	//	m_deferredCommandsList.push_back(std::move(resolvePtrAction));
	//}

	return result;
}

JsonReader::ReadResult JsonReader::ReadProxy(rttr::TypeProxyData* proxyTypeData, void* value, const Json::Value& jsonVal)
{
	ReadResult result = k_readFailedGeneric;

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

JsonReader::ReadResult JsonReader::ReadArray(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	ReadResult result = k_readFailedGeneric;

	if (jsonVal.isArray())
	{
		result = k_readSuccess;

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

JsonReader::ReadResult JsonReader::ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	assert(m_isOk);

	ReadResult result = k_readFailedGeneric;

	// Find in predefined types list
	auto predefinedTypeIt = m_predefinedJsonTypeResolvers.find(type.GetTypeIndex());
	if (predefinedTypeIt != m_predefinedJsonTypeResolvers.end())
	{
		predefinedTypeIt->second->Read(type, value, jsonVal);
		result = k_readSuccess; // Check errors from predefined types
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

				auto resolverAction = std::make_unique<detail::CustomResolverAction>(m_contextPath.GetSize()
					, type, value, serializedValueType, serializedValue, customTypeResolver);
				m_deferredCommandsList.push_back(std::move(resolverAction));
			}
			else
			{
				switch (type.GetTypeClass())
				{
					case rttr::TypeClass::Object:
					{
						result = k_readSuccess;

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
						ReadResult pointerReadResult = ReadPointer(type, value, jsonVal);
					
						// If we have read valid pointer, we notify calling code, that in the read chain there is a pointer that must be resolved later
						if (pointerReadResult.Succeeded())
						{
							result.success = true;
							result.allEntitiesResolved = false;
						}
						else
						{
							// If we failed to obtain the pointer description, we mark pointer as nullptr immediately and return not success
							uintptr_t* ptrValue = static_cast<uintptr_t*>(value);
							*ptrValue = 0;

							result.success = false;
							result.allEntitiesResolved = true;
						}
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
							result = k_readSuccess;
						}
						else
						{
							result = k_readFailedGeneric;
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
										result = k_readSuccess;
									}
									break;
								case Json::intValue:
								case Json::uintValue:
								case Json::nullValue:
									{
										*static_cast<bool*>(value) = !!jsonVal.asUInt64();
										result = k_readSuccess;
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

								result = k_readSuccess;
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

								result = k_readSuccess;
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

void JsonReader::AddCustomTypeResolver(const rttr::Type& type, rttr::CustomTypeResolver* resolver)
{
	m_customTypeResolvers.emplace(type.GetTypeIndex(), resolver);
}

std::string JsonReader::GetObjectClassName(const Json::Value& jsonVal) const
{
	if (jsonVal.isObject() && jsonVal.isMember(k_typeId))
	{
		return jsonVal[k_typeId].asString();
	}

	return std::string();
}

rttr::Type JsonReader::DeduceType(const Json::Value& jsonVal) const
{
	switch (jsonVal.type())
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
	}
}

} // namespace rs
