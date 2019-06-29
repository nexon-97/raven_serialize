#include "readers/JsonReader.hpp"
#include "rttr/Property.hpp"
#include "rttr/CustomTypeResolver.hpp"
#include "rttr/Manager.hpp"

namespace
{
const char* k_typeId = "$type$";
const char* k_contextTypeName = "@context@";
const char* k_contextObjectsKey = "$objects$";
const char* k_smartptrTypeKey = "@smartptr@";
const char* k_sysTypeId = "$sys_type$";
const char* k_smartptrPointedValueKey = "ptr_value";
const char* k_ptrMarkerKey = "$marker_id$";
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
}

void JsonReader::ReadObjectWithContext(const rttr::Type& type, void* value)
{
	std::string className = GetObjectClassName(m_jsonRoot);
	if (className == k_contextTypeName)
	{
		m_context = std::make_unique<rs::detail::SerializationContext>();

		// Parse context
		const Json::Value& contextObjects = m_jsonRoot[k_contextObjectsKey];
		std::size_t idx = 0U;
		for (const Json::Value& contextObject : contextObjects)
		{
			rttr::Type objectType = DeduceType(contextObject);
			void* object = nullptr;

			if (idx == 0U)
			{
				object = value;
			}
			else
			{
				object = objectType.Instantiate();
			}

			ReadImpl(objectType, object, contextObject);

			m_context->AddObject(objectType, object);
			++idx;
		}

		// Resolve pointers
		for (const auto& ptrToResolve : m_context->GetPointersToFill())
		{
			auto objectData = m_context->GetObjectById(ptrToResolve.objectId);
			if (nullptr != objectData)
			{
				ptrToResolve.type.AssignPointerValue(ptrToResolve.pointerAddress, const_cast<void*>(objectData->objectPtr));
			}
			else
			{
				ptrToResolve.type.AssignPointerValue(ptrToResolve.pointerAddress, nullptr);
			}
		}
	}
	else
	{
		ReadImpl(type, value, m_jsonRoot);
	}
}

void JsonReader::Read(const rttr::Type& type, void* value)
{
	//ReadImpl(type, value, jsonValue);
}

void JsonReader::ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	assert(m_isOk);

	// Find custom type resolver for this type
	rttr::CustomTypeResolver* customTypeResolver = nullptr;
	auto customResolverIt = m_customTypeResolvers.find(type.GetTypeIndex());
	if (customResolverIt != m_customTypeResolvers.end())
	{
		customTypeResolver = customResolverIt->second;
	}

	if (nullptr != customTypeResolver)
	{
		// Serialized value type
		rttr::Type serializedValueType = DeduceType(jsonVal);
		void* serializedValue = nullptr;

		// Deserialize custom value from data source to pass to resolver
		if (serializedValueType.GetTypeIndex() != typeid(nullptr_t))
		{
			serializedValue = serializedValueType.Instantiate();
			ReadImpl(serializedValueType, serializedValue, jsonVal);
		}

		// Convert address to variable to pointer-to-pointer
		std::uintptr_t* pointerAddress = reinterpret_cast<std::uintptr_t*>(value);
		auto resolveResult = customTypeResolver->ResolveReverse(type, serializedValueType, pointerAddress, serializedValue);
	}
	else if (type.GetTypeIndex() == typeid(nullptr_t))
	{
		// Skip nullptr
	}
	else if (type.GetTypeIndex() == typeid(bool))
	{
		*static_cast<bool*>(value) = jsonVal.asBool();
	}
	else if (type.IsString())
	{
		if (jsonVal.isNull())
		{
			if (type.GetTypeIndex() == typeid(std::string))
			{
				*static_cast<std::string*>(value) = std::string();
			}
			else if (type.GetTypeIndex() == typeid(const char*))
			{
				char** strSerializedValue = reinterpret_cast<char**>(value);
				*strSerializedValue = nullptr;
			}
		}
		else if (jsonVal.isString())
		{
			const char* strValue = jsonVal.asCString();

			if (type.GetTypeIndex() == typeid(std::string))
			{
				*static_cast<std::string*>(value) = strValue;
			}
			else if (type.GetTypeIndex() == typeid(const char*))
			{
				char** strSerializedValue = reinterpret_cast<char**>(value);
				*strSerializedValue = const_cast<char*>(strValue);
			}
		}
	}
	else if (type.IsIntegral())
	{
		if (type.IsSigned())
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
		}
	}
	else if (type.IsFloatingPoint())
	{
		double floatValue = jsonVal.asDouble();

		if (type.GetTypeIndex() == typeid(float))
		{
			*static_cast<float*>(value) = static_cast<float>(floatValue);
		}
		else
		{
			*static_cast<double*>(value) = floatValue;
		}
	}
	else if (type.IsSmartPointer())
	{
		const Json::Value& smartPtrValue = jsonVal[k_smartptrPointedValueKey];
		if (nullptr != m_context && smartPtrValue.isObject())
		{
			auto markerId = smartPtrValue[k_ptrMarkerKey].asUInt();
			m_context->AddPointerToFill(type, markerId, value);
		}
	}
	else if (type.IsArray())
	{
		// Resize dynamic array
		if (type.IsDynamicArray())
		{
			std::size_t count = jsonVal.size();
			type.SetDynamicArraySize(value, count);
		}

		auto itemType = type.GetUnderlyingType();

		std::size_t i = 0U;
		for (const auto& jsonItem : jsonVal)
		{
			void* itemValuePtr = type.GetArrayItemValuePtr(value, i);

			ReadImpl(itemType, itemValuePtr, jsonItem);

			++i;
		}

		// [DEBUG]
		type.GetDynamicArraySize(value);
	}
	else if (type.IsClass())
	{
		std::size_t propertiesCount = type.GetPropertiesCount();
		for (std::size_t i = 0U; i < propertiesCount; ++i)
		{
			auto property = type.GetProperty(i);
			const rttr::Type& propertyType = property->GetType();
			const char* propertyName = property->GetName();

			const auto& itemJsonVal = jsonVal[propertyName];

			void* valuePtr = nullptr;
			bool needRelease = false;
			property->GetMutatorContext(value, valuePtr, needRelease);

			ReadImpl(propertyType, valuePtr, itemJsonVal);
			property->CallMutator(value, valuePtr);

			if (needRelease)
			{
				delete valuePtr;
			}
		}
	}
	else if (type.IsPointer())
	{
		if (nullptr != m_context)
		{
			auto markerId = jsonVal[k_ptrMarkerKey].asUInt();
			m_context->AddPointerToFill(type, markerId, value);
		}
	}
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
					return rttr::Reflect(typeId.c_str());
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

}
