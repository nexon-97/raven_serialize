#include "writers/JsonReader.hpp"
#include "rttr/Property.hpp"
#include "rttr/PointerTypeResolver.hpp"
#include "rttr/Manager.hpp"

namespace
{
const char* k_typeId = "$type$";
}

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

	delete[] buffer;
}

void JsonReader::Read(const rttr::Type& type, void* value)
{
	ReadImpl(type, value, m_jsonRoot);
}

void* JsonReader::Read(const rttr::Type& type)
{
	void* value = type.Instantiate();
	ReadImpl(type, value, m_jsonRoot);

	return value;
}

void JsonReader::ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	assert(m_isOk);

	if (type.GetTypeIndex() == typeid(bool))
	{
		*static_cast<bool*>(value) = jsonVal.asBool();
	}
	else if (type.IsString())
	{
		auto strValue = jsonVal.asCString();

		if (type.GetTypeIndex() == typeid(std::string))
		{
			*static_cast<std::string*>(value) = strValue;
		}
		else if (type.GetTypeIndex() == typeid(const char*))
		{
			//*const_cast<char*>(static_cast<const char*>(value)) = const_cast<char*>(strValue);
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
	}
	else if (type.IsClass())
	{
		std::size_t propertiesCount = type.GetPropertiesCount();
		for (std::size_t i = 0U; i < propertiesCount; ++i)
		{
			auto property = type.GetProperty(i);
			const rttr::Type& propertyType = property->GetType();

			const auto& itemJsonVal = jsonVal[property->GetName()];

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
		auto pointedType = type.GetUnderlyingType(0U);

		rttr::PointerTypeResolver* resolver = nullptr;
		bool pointerResolved = false;

		auto customResolverIt = m_customPointerTypeResolvers.find(pointedType.GetTypeIndex());
		if (customResolverIt != m_customPointerTypeResolvers.end())
		{
			resolver = customResolverIt->second;
		}

		if (nullptr == resolver)
		{
			// Try default resolver here
		}

		// Serialized value type
		rttr::Type serializedValueType = DeduceType(jsonVal);
		void* serializedValue = serializedValueType.Instantiate();
		if (serializedValueType.GetTypeIndex() == typeid(const char*))
		{
			char** strSerializedValue = reinterpret_cast<char**>(serializedValue);
			*strSerializedValue = const_cast<char*>(jsonVal.asCString());
		}

		if (nullptr != resolver)
		{
			// Convert address to variable to pointer-to-pointer
			std::intptr_t* pointerAddress = reinterpret_cast<std::intptr_t*>(value);
			auto resolveResult = resolver->ResolveReverse(pointedType, serializedValueType, pointerAddress, serializedValue);
		}
	}
}

bool JsonReader::IsOk() const
{
	return m_isOk;
}

void JsonReader::AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver)
{
	m_customPointerTypeResolvers.emplace(type.GetTypeIndex(), resolver);
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
			return rttr::Reflect(jsonVal[k_typeId].asCString());
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
