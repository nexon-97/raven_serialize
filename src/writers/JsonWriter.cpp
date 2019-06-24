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
const char* k_ptrClassKey = "$ptr_type$";
const char* k_ptrTypeValue = "@ptr@";
const char* k_smartptrTypeKey = "@smartptr@";
const char* k_smartptrPointedTypeKey = "$smartptr_type$";
const char* k_smartptrPointedValueKey = "ptr_value";
const char* k_sharedPtrTypeName = "@shared_ptr@";
const char* k_uniquePtrTypeName = "@unique_ptr@";
const char k_stringParethesis = '"';

}

JsonWriter::JsonWriter(std::ostream& stream, const bool prettyPrint)
	: m_stream(stream)
	, m_prettyPrint(prettyPrint)
{}

void JsonWriter::Write(const rttr::Type& type, const void* value)
{
	Json::Value& jsonValue = *m_jsonStack.top();

	if (nullptr == value)
	{
		jsonValue = Json::Value(Json::nullValue);
	}
	else if (type.GetTypeIndex() == typeid(bool))
	{
		bool boolValue = *static_cast<const bool*>(value);
		jsonValue = Json::Value(boolValue);
	}
	else if (type.IsString())
	{
		if (type.GetTypeIndex() == typeid(std::string))
		{
			const std::string* stringPtr = reinterpret_cast<const std::string*>(value);
			jsonValue = Json::Value(*stringPtr);
		}
		else if (type.GetTypeIndex() == typeid(std::wstring))
		{
			const std::wstring* stringPtr = reinterpret_cast<const std::wstring*>(value);
			std::string utf8String = WStringToUtf8(stringPtr->c_str());
			jsonValue = Json::Value(utf8String);
		}
		else if (type.GetTypeIndex() == typeid(const char*))
		{
			char* const* cStringPtr = reinterpret_cast<char* const*>(value);
			char* cStringVal = *cStringPtr;

			if (nullptr == cStringVal)
			{
				jsonValue = Json::Value(Json::nullValue);
			}
			else
			{
				jsonValue = Json::Value(cStringVal);
			}
		}
	}
	else if (type.IsIntegral())
	{
		if (type.IsSigned())
		{
			int64_t intValue = type.CastToSignedInteger(value);
			jsonValue = Json::Value(intValue);
		}
		else
		{
			uint64_t intValue = type.CastToUnsignedInteger(value);
			jsonValue = Json::Value(intValue);
		}
	}
	else if (type.IsSmartPointer())
	{
		auto pointerType = type.GetUnderlyingType(0U);
		void* smartPtrValue = type.GetSmartPtrValue(const_cast<void*>(value));

		jsonValue = Json::Value(Json::objectValue);
		jsonValue[k_typeId] = k_smartptrTypeKey;
		jsonValue[k_smartptrPointedTypeKey] = pointerType.GetName();
		jsonValue[k_smartptrPointedValueKey] = Json::Value();
	
		m_jsonStack.push(&jsonValue[k_smartptrPointedValueKey]);
		Write(pointerType, &smartPtrValue);
		m_jsonStack.pop();
	}
	else if (type.IsEnum())
	{
		const rttr::Type& enumUnderlyingType = type.GetUnderlyingType(0U);
		Write(enumUnderlyingType, value);
	}
	else if (type.IsFloatingPoint())
	{
		double floatValue = type.CastToFloat(value);
		jsonValue = Json::Value(floatValue);
	}
	else if (type.IsArray())
	{
		jsonValue = Json::Value(Json::arrayValue);

		if (type.IsDynamicArray())
		{
			std::size_t arraySize = type.GetDynamicArraySize(value);

			auto f = [this, arraySize, &jsonValue](const rttr::Type& itemType, std::size_t index, const void* itemPtr)
			{
				Json::Value itemJson;

				m_jsonStack.push(&itemJson);
				Write(itemType, itemPtr);
				m_jsonStack.pop();

				jsonValue.append(itemJson);
			};
			type.IterateArray(value, f);
		}
		else
		{
			for (std::size_t dim = 0U; dim < type.GetArrayRank(); ++dim)
			{
				std::size_t arraySize = type.GetArrayExtent(dim);

				auto f = [this, arraySize, &jsonValue](const rttr::Type& itemType, std::size_t index, const void* itemPtr)
				{
					Json::Value itemJson;

					m_jsonStack.push(&itemJson);
					Write(itemType, itemPtr);
					m_jsonStack.pop();

					jsonValue.append(itemJson);
				};

				type.IterateArray(value, f);
			}
		}
	}
	else if (type.IsClass())
	{
		jsonValue = Json::Value(Json::objectValue);

		std::size_t propertiesCount = type.GetPropertiesCount();

		// Write metaclass names
		jsonValue[k_typeId] = type.GetName();

		for (std::size_t i = 0U; i < propertiesCount; ++i)
		{
			auto property = type.GetProperty(i);
			const rttr::Type& propertyType = property->GetType();

			const char* propertyName = property->GetName();
			jsonValue[propertyName] = Json::Value();
			m_jsonStack.push(&jsonValue[propertyName]);

			void* valuePtr = nullptr;
			bool needRelease = false;
			property->GetValue(value, valuePtr, needRelease);

			Write(propertyType, valuePtr);
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

		if (nullptr != resolver)
		{
			// Convert address to variable to pointer-to-pointer
			const std::uintptr_t* pointerAddress = reinterpret_cast<const std::uintptr_t*>(value);
			// Deference void* as pointer value
			std::uintptr_t pointerValue = *pointerAddress;
			// Interpret resolved pointer value as new pointer to void
			const void* pointedAddress = reinterpret_cast<const void*>(pointerValue);

			auto resolveResult = resolver->Resolve(pointedType, pointedAddress);

			if (nullptr != resolveResult && resolveResult->resolved)
			{
				pointerResolved = true;
				Write(resolveResult->resolvedType, resolveResult->resolvedValue);
			}
		}

		if (!pointerResolved)
		{
			// No resolver found, so put null here
			rttr::Type pointedMetaType = rttr::Reflect(pointerTypeIndex);
			if (pointedMetaType.IsValid())
			{
				const char* typeName = pointedMetaType.GetName();

				jsonValue = Json::Value(Json::objectValue);
				jsonValue[k_typeId] = k_ptrTypeValue;
				jsonValue[k_ptrClassKey] = typeName;
			}
			else
			{
				jsonValue = Json::Value(Json::nullValue);
			}
		}
	}
	else
	{
		// This type of meta-type is not supported?
		jsonValue = Json::Value(Json::nullValue);
	}
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
