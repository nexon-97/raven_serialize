#include "writers/JsonReader.hpp"
#include "rttr/Property.hpp"

JsonReader::JsonReader(std::istream& stream)
	: m_stream(stream)
{
	m_stream.seekg(0, std::ios::end);
	std::size_t bufferSize = static_cast<std::size_t>(m_stream.tellg());
	m_stream.seekg(0, std::ios::beg);
	std::streamoff offset = 0U;

	char* buffer = new char[bufferSize];
	m_stream.read(buffer, bufferSize);

	Json::CharReaderBuilder builder;
	auto reader = builder.newCharReader();
	std::string errorStr;

	if (reader->parse(buffer, buffer + bufferSize, &m_jsonRoot, &errorStr))
	{
		int a = 0;
	}

	delete[] buffer;
}

void JsonReader::Read(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
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

			Read(itemType, itemValuePtr, jsonItem);

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

			Read(propertyType, valuePtr, itemJsonVal);
			property->CallMutator(value, valuePtr);

			if (needRelease)
			{
				delete valuePtr;
			}
		}
	}
}
