#include "writers/StreamJsonWriter.hpp"

namespace rs
{

StreamJsonWriter::StreamJsonWriter(std::ostream& stream, const bool prettyPrint)
	: m_stream(stream)
	, m_prettyPrint(prettyPrint)
{}

bool StreamJsonWriter::Write(const rttr::Type& type, const void* value)
{
	if (m_stream.good())
	{
		// Find in predefined types list
		//auto predefinedTypeIt = g_predefinedJsonTypeResolvers.find(type.GetTypeIndex());
		//if (predefinedTypeIt != g_predefinedJsonTypeResolvers.end())
		//{
		//	predefinedTypeIt->second->Read(type, value, jsonVal);
		//	result = ReadResult::OKResult(); // Check errors from predefined types
		//}
		//else
		//{
			rs::SerializationMethod serializationMethod = type.GetSerializationMethod();

			switch (serializationMethod)
			{
			case rs::SerializationMethod::Proxy:
			{
				//rttr::TypeProxyData* proxyTypeData = rttr::Manager::GetRTTRManager().GetProxyType(type);
				//if (nullptr != proxyTypeData)
				//{
				//	result = ReadProxy(proxyTypeData, value, jsonVal);
				//}
			}
			break;
			case rs::SerializationMethod::Adapter:
			{
				//SerializationAdapter* adapter = rttr::Manager::GetRTTRManager().GetSerializationAdapter(type);
				//if (nullptr != adapter)
				//{
				//	// Parse payload
				//	SerializationAdapter::DataChunk payload;
				//	payload.type = adapter->GetPayloadType();

				//	if (payload.type.IsValid() && jsonVal.isObject() && jsonVal.isMember(K_ADAPTER))
				//	{
				//		// If it's a json object with adapter member, treat it as payload
				//		payload.value = m_context->CreateTempVariable(payload.type);
				//		ReadResult payloadReadResult = ReadImpl(payload.type, payload.value, jsonVal[K_ADAPTER]);
				//	}

				//	// Perform adapter logic (payload can be empty)
				//	SerializationAdapter::AdapterReadOutput adapterOutput = adapter->ReadConvert(payload);

				//	// Read json value as converted type
				//	if (adapterOutput.convertedType.IsValid())
				//	{
				//		void* adapterValue = m_context->CreateTempVariable(adapterOutput.convertedType);
				//		result = ReadImpl(adapterOutput.convertedType, adapterValue, jsonVal);

				//		adapter->ReadFinalize(adapterValue, value, adapterOutput, payload);
				//	}
				//}
			}
			break;
			default:
			{
				switch (type.GetTypeClass())
				{
				case rttr::TypeClass::Object:
				{
					//result = ReadResult::OKResult();

					//ReadResult basesReadResult = ReadObjectBases(type, value, jsonVal);
					//result.Merge(basesReadResult);
					m_stream << '{';

					// Read object properties if any
					const std::size_t propertiesCount = type.GetPropertiesCount();
					if (propertiesCount > 0U)
					{
						//ReadResult propertiesReadResult = ReadObjectProperties(type, value, jsonVal, propertiesCount);
						//result.Merge(propertiesReadResult);
					}

					// Read collection items if this type is a collection
					if (type.IsCollection())
					{
						//ReadResult collectionReadResult = ReadCollection(type, value, jsonVal, propertiesCount);
						//result.Merge(collectionReadResult);
					}

					m_stream << '}';
				}
				break;
				case rttr::TypeClass::Pointer:
				{
					//result = ReadPointer(type, value, jsonVal);
				}
				break;
				case rttr::TypeClass::Enum:
				{
					//rttr::Type enumUnderlyingType = type.GetEnumUnderlyingType();
					//result = ReadImpl(enumUnderlyingType, value, jsonVal);
				}
				break;
				case rttr::TypeClass::Real:
				{
					//double valueToAssign = 0.0;

					//if (jsonVal.isNumeric())
					//{
					//	valueToAssign = jsonVal.asDouble();
					//	result = ReadResult::OKResult();
					//}
					//else
					//{
					//	result = ReadResult::GenericFailResult();
					//}

					//if (type.GetTypeIndex() == typeid(float))
					//{
					//	*static_cast<float*>(value) = static_cast<float>(valueToAssign);
					//}
					//else
					//{
					//	*static_cast<double*>(value) = valueToAssign;
					//}
				}
				break;
				case rttr::TypeClass::Integral:
				{
					//if (type.GetTypeIndex() == typeid(bool))
					//{
					//	switch (jsonVal.type())
					//	{
					//	case Json::booleanValue:
					//	{
					//		*static_cast<bool*>(value) = jsonVal.asBool();
					//		result = ReadResult::OKResult();
					//	}
					//	break;
					//	case Json::intValue:
					//	case Json::uintValue:
					//	case Json::nullValue:
					//	{
					//		*static_cast<bool*>(value) = !!jsonVal.asUInt64();
					//		result = ReadResult::OKResult();
					//	}
					//	break;
					//	default:
					//		break;
					//	}
					//}
					//else
					//{
					//	if (type.IsSignedIntegral())
					//	{
					//		int64_t intValue = jsonVal.asInt64();

					//		switch (type.GetSize())
					//		{
					//		case 1:
					//			*static_cast<int8_t*>(value) = static_cast<int8_t>(intValue);
					//			break;
					//		case 2:
					//			*static_cast<int16_t*>(value) = static_cast<int16_t>(intValue);
					//			break;
					//		case 4:
					//			*static_cast<int32_t*>(value) = static_cast<int32_t>(intValue);
					//			break;
					//		case 8:
					//		default:
					//			*static_cast<int64_t*>(value) = intValue;
					//			break;
					//		}

					//		result = ReadResult::OKResult();
					//	}
					//	else
					//	{
					//		uint64_t intValue = jsonVal.asUInt64();

					//		switch (type.GetSize())
					//		{
					//		case 1:
					//			*static_cast<uint8_t*>(value) = static_cast<uint8_t>(intValue);
					//			break;
					//		case 2:
					//			*static_cast<uint16_t*>(value) = static_cast<uint16_t>(intValue);
					//			break;
					//		case 4:
					//			*static_cast<uint32_t*>(value) = static_cast<uint32_t>(intValue);
					//			break;
					//		case 8:
					//		default:
					//			*static_cast<uint64_t*>(value) = intValue;
					//			break;
					//		}

					//		result = ReadResult::OKResult();
					//	}
					//}
				}
				break;
				case rttr::TypeClass::Array:
				{
					//result = ReadArray(type, value, jsonVal);
				}
				break;
				}
			}
			break;
			}

		//}

		return true;
	}

	return false;
}

void StreamJsonWriter::WritePadding()
{
	for (int i = 0; i < m_padding; i++)
	{
		m_stream << '\t';
	}
}

}
