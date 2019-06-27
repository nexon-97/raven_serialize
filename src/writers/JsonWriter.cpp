#include "writers/JsonWriter.hpp"
#include "rttr/Manager.hpp"
#include "rttr/Property.hpp"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>

namespace
{

const char* k_true = "true";
const char* k_false = "false";
const char* k_null = "null";
const char* k_typeId = "$type$";
const char* k_sysTypeId = "$sys_type$";
const char* k_ptrClassKey = "$ptr_type$";
const char* k_ptrMarkerKey = "$marker_id$";
const char* k_ptrTypeValue = "@ptr@";
const char* k_smartptrTypeKey = "@smartptr@";
const char* k_smartptrTypeSpecializationKey = "$smartptr_type$";
const char* k_smartptrUnderlyingTypeKey = "$smartptr_sub_type$";
const char* k_smartptrPointedValueKey = "ptr_value";
const char* k_sharedPtrTypeName = "@shared_ptr@";
const char* k_uniquePtrTypeName = "@unique_ptr@";
const char* k_contextTypeName = "@context@";
const char* k_contextObjectsKey = "$objects$";
const char k_stringParethesis = '"';

}

namespace rs
{

JsonWriter::JsonWriter(std::ostream& stream, const bool prettyPrint)
	: m_stream(stream)
	, m_prettyPrint(prettyPrint)
{}

void JsonWriter::WriteObject(const rttr::Type& type, const void* value)
{
	if (nullptr == value)
	{
		Json::Value& nullValue = CreateJsonObject(Json::nullValue);
	}
	else if (type.GetTypeIndex() == typeid(bool))
	{
		Json::Value& jsonBool = CreateJsonObject(Json::booleanValue);
		bool boolValue = *static_cast<const bool*>(value);
		jsonBool = Json::Value(boolValue);
	}
	else if (type.IsString())
	{
		Json::Value& jsonStr = CreateJsonObject(Json::stringValue);

		if (type.GetTypeIndex() == typeid(std::string))
		{
			const std::string* stringPtr = reinterpret_cast<const std::string*>(value);
			jsonStr = Json::Value(*stringPtr);
		}
		else if (type.GetTypeIndex() == typeid(std::wstring))
		{
			const std::wstring* stringPtr = reinterpret_cast<const std::wstring*>(value);
			std::string utf8String = WStringToUtf8(stringPtr->c_str());
			jsonStr = Json::Value(utf8String);
		}
		else if (type.GetTypeIndex() == typeid(const char*))
		{
			char* const* cStringPtr = reinterpret_cast<char* const*>(value);
			char* cStringVal = *cStringPtr;

			if (nullptr == cStringVal)
			{
				jsonStr = Json::Value(Json::nullValue);
			}
			else
			{
				jsonStr = Json::Value(cStringVal);
			}
		}
	}
	else if (type.IsIntegral())
	{
		if (type.IsSigned())
		{
			Json::Value& jsonInt = CreateJsonObject(Json::intValue);
			int64_t intValue = type.CastToSignedInteger(value);
			jsonInt = Json::Value(intValue);
		}
		else
		{
			Json::Value& jsonUInt = CreateJsonObject(Json::intValue);
			uint64_t uintValue = type.CastToUnsignedInteger(value);
			jsonUInt = Json::Value(uintValue);
		}
	}
	else if (type.IsSmartPointer())
	{
		auto pointerType = type.GetUnderlyingType(0U);
		void* smartPtrValue = type.GetSmartPtrValue(const_cast<void*>(value));

		Json::Value& smartptrJsonValue = CreateJsonObject(Json::objectValue);
		smartptrJsonValue[k_typeId] = k_smartptrTypeKey;
		smartptrJsonValue[k_smartptrTypeSpecializationKey] = type.GetSmartPtrTypeName();
		smartptrJsonValue[k_smartptrUnderlyingTypeKey] = pointerType.GetName();
		smartptrJsonValue[k_sysTypeId] = type.GetName();

		WriteObject(pointerType, &smartPtrValue);
		smartptrJsonValue[k_smartptrPointedValueKey] = *m_jsonStack.top();
		m_jsonStack.pop();
	}
	else if (type.IsEnum())
	{
		const rttr::Type& enumUnderlyingType = type.GetUnderlyingType(0U);
		WriteObject(enumUnderlyingType, value);
	}
	else if (type.IsFloatingPoint())
	{
		Json::Value& jsonReal = CreateJsonObject(Json::realValue);
		double floatValue = type.CastToFloat(value);
		jsonReal = Json::Value(floatValue);
	}
	else if (type.IsArray())
	{
		if (type.IsDynamicArray())
		{
			std::size_t arraySize = type.GetDynamicArraySize(value);
			void* itemsPointer = type.GetArrayItemValuePtr(const_cast<void*>(value), 0U);
			WriteArray(type.GetUnderlyingType(0U), itemsPointer, arraySize);
		}
		else
		{
			std::size_t arraySize = type.GetArrayExtent(0U);
			WriteArray(type.GetUnderlyingType(0U), value, arraySize);
		}
	}
	else if (type.IsClass())
	{
		Json::Value& jsonClass = CreateJsonObject(Json::objectValue);

		std::size_t propertiesCount = type.GetPropertiesCount();

		// Write metaclass names
		jsonClass[k_typeId] = type.GetName();

		for (std::size_t i = 0U; i < propertiesCount; ++i)
		{
			auto property = type.GetProperty(i);
			const rttr::Type& propertyType = property->GetType();

			const char* propertyName = property->GetName();

			void* valuePtr = nullptr;
			bool needRelease = false;
			property->GetValue(value, valuePtr, needRelease);

			WriteObject(propertyType, valuePtr);
			jsonClass[propertyName] = *m_jsonStack.top();
			m_jsonStack.pop();

			if (needRelease)
			{
				delete valuePtr;
			}
		}
	}
	else if (type.IsPointer())
	{
		auto pointedType = type.GetUnderlyingType(0U);
		auto pointerTypeIndex = type.GetPointerTypeIndex(const_cast<void*>(value));

		rttr::PointerTypeResolver* resolver = nullptr;
		bool pointerResolved = false;

		auto customResolverIt = m_customPointerTypeResolvers.find(pointerTypeIndex);
		if (customResolverIt != m_customPointerTypeResolvers.end())
		{
			resolver = customResolverIt->second;
		}

		if (nullptr == resolver)
		{
			// Try default resolver
		}

		std::unique_ptr<rttr::PointerTypeResolver::ResolveResult> resolveResult;
		if (nullptr != resolver)
		{
			// Convert address to variable to pointer-to-pointer
			const std::uintptr_t* pointerAddress = reinterpret_cast<const std::uintptr_t*>(value);
			// Deference void* as pointer value
			std::uintptr_t pointerValue = *pointerAddress;
			// Interpret resolved pointer value as new pointer to void
			const void* pointedAddress = reinterpret_cast<const void*>(pointerValue);

			resolveResult = resolver->Resolve(pointedType, pointedAddress);
			pointerResolved = true;

			if (nullptr != resolveResult && resolveResult->resolved)
			{
				WriteObject(resolveResult->resolvedType, resolveResult->resolvedValue);
			}
			else
			{
				// Custom resolver didn't resolve the pointer, so put null here
				Json::Value& jsonNull = CreateJsonObject(Json::nullValue);
			}
		}

		if (!pointerResolved)
		{
			// Pointers can have no custom resolvers, so must serialize them using serialization context
			rttr::Type pointedMetaType = rttr::Reflect(pointerTypeIndex);
			if (pointedMetaType.IsValid())
			{
				CreateSerializationContext();

				const void* const* ptrToValue = reinterpret_cast<const void* const*>(value);
				std::size_t objectId = m_context->AddObject(pointedMetaType, *ptrToValue);
				Json::Value& jsonValue = CreateJsonObject(Json::objectValue);
				jsonValue[k_typeId] = type.GetName();
				jsonValue[k_ptrMarkerKey] = objectId;
			}
			else
			{
				Json::Value& jsonNull = CreateJsonObject(Json::nullValue);
			}
		}
	}
	else
	{
		// This type is not supported?
		Json::Value& jsonNull = CreateJsonObject(Json::nullValue);
	}
}

void JsonWriter::WriteArray(const rttr::Type& itemType, const void* arrayStartPtr, const std::size_t size)
{
	// Create array json value to store the array
	Json::Value& arrayValue = CreateJsonObject(Json::arrayValue);

	const std::size_t itemSize = itemType.GetSize();
	const uint8_t* arrayDataPtr = reinterpret_cast<const uint8_t*>(arrayStartPtr);
	for (std::size_t i = 0U; i < size; ++i)
	{
		WriteObject(itemType, arrayDataPtr + itemSize * i);
		Json::Value& itemValue = *m_jsonStack.top();
		arrayValue.append(itemValue);

		m_jsonStack.pop();
	}
}

void JsonWriter::WriteCustomObject(CustomObjectWriterFunc* objectWritersPtr, const std::size_t writersCount)
{
	/*Json::Value& jsonValue = *m_jsonStack.top();
	jsonValue = Json::Value(Json::objectValue);

	for (std::size_t i = 0U; i < writersCount; ++i)
	{
		Json::Value& itemValue = CreateJsonObject(Json::nullValue);
		CustomObjectWriterFunc& objectWriter = *(objectWritersPtr + i);
		std::invoke(objectWriter, this);
		m_jsonStack.pop();

		jsonValue.append(std::move(itemValue));
	}*/
}

std::string JsonWriter::WStringToUtf8(const wchar_t* _literal)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string utf8String = converter.to_bytes(_literal);

	return utf8String;
}

void JsonWriter::AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver)
{
	m_customPointerTypeResolvers.emplace(type.GetTypeIndex(), resolver);
}

void JsonWriter::CreateSerializationContext()
{
	if (nullptr == m_context)
	{
		m_context = std::make_unique<rs::detail::SerializationContext>();
		m_isUsingPointerContext = true;
	}
}

void JsonWriter::DoWrite()
{
	Json::Value& serializedValue = m_jsonRoot;

	GenerateSerializationContextValues();

	if (!m_serializedObjects.empty())
	{
		Json::Value contextWrapper = Json::Value(Json::objectValue);
		contextWrapper[k_typeId] = k_contextTypeName;
		contextWrapper[k_contextObjectsKey] = Json::Value(Json::arrayValue);
		Json::Value& contextObjects = contextWrapper[k_contextObjectsKey];

		contextObjects.append(m_jsonRoot);
		for (auto& item : m_serializedObjects)
		{
			contextObjects.append(std::move(item));
		}

		serializedValue = contextWrapper;
	}

	Json::StreamWriterBuilder writerBuilder;

	if (m_prettyPrint)
	{
		writerBuilder["indentation"] = "\t";
	}
	else
	{
		writerBuilder["indentation"] = "";
	}

	Json::StreamWriter* jsonWriter = writerBuilder.newStreamWriter();
	jsonWriter->write(serializedValue, &m_stream);
}

void JsonWriter::GenerateSerializationContextValues()
{
	if (nullptr == m_context)
		return;

	m_serializedObjects.clear();
	for (const auto& contextValue : m_context->GetObjects())
	{
		WriteObject(contextValue.type, contextValue.objectPtr);
		Json::Value& objectJsonValue = *m_jsonStack.top();
		objectJsonValue[k_ptrMarkerKey] = contextValue.objectId;
		m_jsonStack.pop();

		m_serializedObjects.push_back(std::move(objectJsonValue));
	}
}

Json::Value& JsonWriter::CreateJsonObject(const Json::ValueType valueType)
{
	m_contextObjects.emplace_back(std::make_unique<Json::Value>(valueType));
	m_jsonStack.push(m_contextObjects.back().get());

	return *m_contextObjects.back();
}

} // namespace rs
