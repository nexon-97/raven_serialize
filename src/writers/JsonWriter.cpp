#include "writers/JsonWriter.hpp"
#include "rttr/Manager.hpp"
#include "rs/SerializationKeywords.hpp"

namespace
{

using PredefinedTypeWriter = std::function<Json::Value(const rttr::Type&, const void*)>;
const std::unordered_map<std::type_index, PredefinedTypeWriter> gPredefinedWriters = {
	{
		typeid(std::string), [](const rttr::Type& type, const void* value) -> Json::Value {
			const std::string& str = *static_cast<const std::string*>(value);
			return Json::Value(str.c_str());
		}
	},
	{
		typeid(const char*), [](const rttr::Type& type, const void* value) -> Json::Value {
			const char* str = static_cast<const char*>(*reinterpret_cast<const void* const*>(value));
			return Json::Value(str);
		}
	},
};

}

namespace rs
{

const Json::Value& JsonWriter::GetJsonValue() const
{
	return m_jsonRoot;
}

bool JsonWriter::Write(const rttr::Type& type, const void* value)
{
	if (type.IsValid() && value)
	{
		m_jsonRoot = WriteInternal(type, value);
		return true;
	}

	return false;
}


Json::Value JsonWriter::WriteInternal(const rttr::Type& type, const void* value)
{
	// Find in predefined types list
	auto predefinedTypeIt = gPredefinedWriters.find(type.GetTypeIndex());
	if (predefinedTypeIt != gPredefinedWriters.end())
	{
		return predefinedTypeIt->second(type, value);
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
				return WriteProxy(proxyTypeData, value);
			}
		}
		break;
		case rs::SerializationMethod::Adapter:
		{
			SerializationAdapter* adapter = rttr::Manager::GetRTTRManager().GetSerializationAdapter(type);
			if (nullptr != adapter)
			{
				// Perform adapter logic, that generates optional payload and actual value
				SerializationAdapter::AdapterWriteOutput adapterOutput = adapter->Write(value);

				Json::Value adapterObject = Json::Value(Json::ValueType::objectValue);

				// Write adapter value
				if (adapterOutput.value.type.IsValid() && adapterOutput.value.value)
				{
					adapterObject = WriteInternal(adapterOutput.value.type, adapterOutput.value.value);
				}

				// Attach payload if present
				if (adapterOutput.payload.type.IsValid() && adapterObject.isObject())
				{
					// Write payload value
					adapterObject[K_ADAPTER] = WriteInternal(adapterOutput.payload.type, adapterOutput.payload.value);
				}

				adapter->WriteFinalize(value);

				return adapterObject;
			}
		}
		break;
		default:
		{
			switch (type.GetTypeClass())
			{
			case rttr::TypeClass::Object:
			{
				const std::size_t propertiesCount = type.GetPropertiesCount();

				Json::Value jsonObject = Json::Value(Json::ValueType::objectValue);
				if (propertiesCount == 0U && type.IsCollection())
				{
					jsonObject = Json::Value(Json::ValueType::arrayValue);
				}

				// [TODO] Add support for base classes parts

				// Read object properties if any
				if (propertiesCount > 0U)
				{
					WriteObjectProperties(type, value, jsonObject);
				}

				// Write collection items if this type is a collection
				if (type.IsCollection())
				{
					Json::Value dedicatedArrayValue(Json::ValueType::arrayValue);
					Json::Value* collectionItemsValue = jsonObject.isArray() ? &jsonObject : &dedicatedArrayValue;

					const rttr::Type itemType = type.GetCollectionItemType();
					for (auto it = type.CreateCollectionIterator(const_cast<void*>(value)); *it; ++(*it))
					{
						void* itemValue = *(*it);
						Json::Value itemJson = WriteInternal(itemType, itemValue);
						collectionItemsValue->append(std::move(itemJson));
					}

					if (collectionItemsValue != &jsonObject)
					{
						jsonObject[K_COLLECTION_ITEMS] = std::move(*collectionItemsValue);
					}
				}

				return jsonObject;
			}
			break;
			case rttr::TypeClass::Pointer:
			{
				return Json::Value(Json::ValueType::nullValue);
			}
			break;
			case rttr::TypeClass::Enum:
			{
				const rttr::Type enumUnderlyingType = type.GetEnumUnderlyingType();
				return WriteInternal(enumUnderlyingType, value);
			}
			break;
			case rttr::TypeClass::Real:
			{
				if (type.GetTypeIndex() == typeid(float))
				{
					return Json::Value(*static_cast<const float*>(value));
				}
				else
				{
					return Json::Value(*static_cast<const double*>(value));
				}
			}
			break;
			case rttr::TypeClass::Integral:
			{
				if (type.GetTypeIndex() == typeid(bool))
				{
					return Json::Value(*static_cast<const bool*>(value));
				}
				else
				{
					if (type.IsSignedIntegral())
					{
						switch (type.GetSize())
						{
						case 1:
							return Json::Value(*static_cast<const int8_t*>(value));
						case 2:
							return Json::Value(*static_cast<const int16_t*>(value));
						case 4:
							return Json::Value(*static_cast<const int32_t*>(value));
						case 8:
						default:
							return Json::Value(*static_cast<const int64_t*>(value));
						}
					}
					else
					{
						switch (type.GetSize())
						{
						case 1:
							return Json::Value(*static_cast<const uint8_t*>(value));
						case 2:
							return Json::Value(*static_cast<const uint16_t*>(value));
						case 4:
							return Json::Value(*static_cast<const uint32_t*>(value));
						case 8:
						default:
							return Json::Value(*static_cast<const uint64_t*>(value));
						}
					}
				}
			}
			break;
			case rttr::TypeClass::Array:
			{
				return WriteArray(type, value);
			}
			break;
			}
		}
		break;
		}
	}

	return Json::Value(Json::ValueType::nullValue);
}

void JsonWriter::WriteObjectProperties(const rttr::Type& type, const void* value, Json::Value& jsonObject)
{
	const std::size_t propertiesCount = type.GetPropertiesCount();
	for (std::size_t i = 0U; i < propertiesCount; i++)
	{
		rttr::Property* const prop = type.GetProperty(i);

		void* propValue = nullptr;
		bool needRelease = false;
		prop->GetValue(value, propValue, needRelease);

		Json::Value propertyValueJson = WriteInternal(prop->GetType(), propValue);
		jsonObject[prop->GetName()] = std::move(propertyValueJson);

		// Release temp object if required
		if (needRelease)
		{
			prop->GetType().Destroy(propValue);
		}
	}
}

Json::Value JsonWriter::WriteProxy(rttr::TypeProxyData* proxyTypeData, const void* value)
{
	if (proxyTypeData->writeConverter)
	{
		// Create proxy object
		void* targetObject = m_context->CreateTempVariable(proxyTypeData->proxyType);

		// Create target object using proxy constructor
		proxyTypeData->writeConverter->Convert(targetObject, value);

		// Write converted object
		return WriteInternal(proxyTypeData->proxyType, targetObject);
	}
	
	return Json::Value(Json::ValueType::nullValue);
}

Json::Value JsonWriter::WriteArray(const rttr::Type& type, const void* value)
{
	Json::Value outJsonValue(Json::ValueType::arrayValue);

	const rttr::Type arrayType = type.GetArrayType();
	const uint8_t* arrayBytePtr = static_cast<const uint8_t*>(value);
	const std::size_t itemSize = arrayType.GetSize();

	std::size_t totalSize = type.GetArrayExtent(0U);
	for (std::size_t i = 1U; i < type.GetArrayRank(); ++i)
	{
		totalSize *= type.GetArrayExtent(i);
	}

	for (std::size_t i = 0U; i < totalSize; i++)
	{
		const uint8_t* itemPtr = arrayBytePtr + itemSize * i;
		Json::Value itemJsonValue = WriteInternal(arrayType, itemPtr);
		outJsonValue.append(std::move(itemJsonValue));
	}

	return outJsonValue;
}

} // namespace rs
