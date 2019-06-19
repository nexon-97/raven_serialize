#include "writers/JsonWriter.hpp"
#include "rttr/Property.hpp"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>

namespace
{

const char* k_true = "true";
const char* k_false = "false";
const char* k_null = "null";
const char* k_typeId = "$type$";
const char k_stringParethesis = '"';

}

JsonWriter::JsonWriter(std::ostream& stream, const bool prettyPrint)
	: m_stream(stream)
	, m_prettyPrint(prettyPrint)
{}

template <typename T>
void Write(const T& value)
{
	Write(rttr::Reflect<T>(), &value);
}

void JsonWriter::Write(const rttr::Type& type, const void* value)
{
	if (nullptr == value)
	{
		m_stream << k_null;
	}
	else if (type.GetTypeIndex() == typeid(bool))
	{
		bool boolValue = *static_cast<const bool*>(value);
		m_stream << (boolValue ? k_true : k_false);
	}
	else if (type.IsString())
	{
		if (type.GetTypeIndex() == typeid(std::string))
		{
			const std::string* stringPtr = reinterpret_cast<const std::string*>(value);
			WriteStringLiteral(stringPtr->c_str());
		}
		else if (type.GetTypeIndex() == typeid(std::wstring))
		{
			const std::wstring* stringPtr = reinterpret_cast<const std::wstring*>(value);
			WriteStringLiteral(stringPtr->c_str());
		}
		else if (type.GetTypeIndex() == typeid(const char*))
		{
			char* const* cStringPtr = reinterpret_cast<char* const*>(value);
			char* cStringVal = *cStringPtr;

			if (nullptr == cStringVal)
			{
				m_stream << k_null;
			}
			else
			{
				WriteStringLiteral(cStringVal);
			}
		}
	}
	else if (type.IsIntegral())
	{
		if (type.IsSigned())
		{
			int64_t intValue = type.CastToSignedInteger(value);
			m_stream << intValue;
		}
		else
		{
			uint64_t intValue = type.CastToUnsignedInteger(value);
			m_stream << intValue;
		}
	}
	else if (type.IsEnum())
	{
		const rttr::Type& enumUnderlyingType = type.GetUnderlyingType(0U);
		Write(enumUnderlyingType, value);
	}
	else if (type.IsFloatingPoint())
	{
		double floatValue = type.CastToFloat(value);
		m_stream << floatValue;
	}
	else if (type.IsArray())
	{
		m_stream << '[';

		if (m_prettyPrint)
		{
			m_stream << std::endl;
		}
		++m_padding;

		if (type.IsDynamicArray())
		{
			std::size_t arraySize = type.GetDynamicArraySize(value);

			auto f = [this, arraySize](const rttr::Type& itemType, std::size_t index, const void* itemPtr)
			{
				if (m_prettyPrint)
				{
					PrintPadding();
				}

				Write(itemType, itemPtr);

				if (index + 1U < arraySize)
				{
					m_stream.put(',');
				}

				if (m_prettyPrint)
				{
					m_stream << std::endl;
				}
			};

			type.IterateArray(value, f);
		}
		else
		{
			for (std::size_t dim = 0U; dim < type.GetArrayRank(); ++dim)
			{
				std::size_t arraySize = type.GetArrayExtent(dim);

				auto f = [this, arraySize](const rttr::Type& itemType, std::size_t index, const void* itemPtr)
				{
					if (m_prettyPrint)
					{
						PrintPadding();
					}

					Write(itemType, itemPtr);

					if (index + 1U < arraySize)
					{
						m_stream.put(',');

					}

					if (m_prettyPrint)
					{
						m_stream << std::endl;
					}
				};

				type.IterateArray(value, f);
			}
		}

		--m_padding;

		if (m_prettyPrint)
		{
			PrintPadding();
		}

		m_stream << ']';
	}
	else if (type.IsClass())
	{
		m_stream << '{';
		if (m_prettyPrint)
		{
			m_stream << std::endl;
		}
		++m_padding;

		std::size_t propertiesCount = type.GetPropertiesCount();

		// Write metaclass names
		{
			if (m_prettyPrint)
			{
				PrintPadding();
			}

			WriteStringLiteral(k_typeId);
			if (m_prettyPrint)
			{
				m_stream << " : ";
			}
			else
			{
				m_stream << ':';
			}
			WriteStringLiteral(type.GetName());

			if (propertiesCount > 0U)
			{
				m_stream << ',';
			}

			if (m_prettyPrint)
			{
				m_stream << std::endl;
			}
		}

		for (std::size_t i = 0U; i < propertiesCount; ++i)
		{
			auto property = type.GetProperty(i);
			const rttr::Type& propertyType = property->GetType();

			if (m_prettyPrint)
			{
				PrintPadding();
			}

			WriteStringLiteral(property->GetName());
			if (m_prettyPrint)
			{
				m_stream << " : ";
			}
			else
			{
				m_stream << ':';
			}

			void* valuePtr = nullptr;
			bool needRelease = false;
			property->GetValue(value, valuePtr, needRelease);

			Write(propertyType, valuePtr);

			if (needRelease)
			{
				delete valuePtr;
			}

			if (i + 1U < propertiesCount)
			{
				m_stream.put(',');
			}

			if (m_prettyPrint)
			{
				m_stream << std::endl;
			}
		}

		--m_padding;
		if (m_prettyPrint)
		{
			PrintPadding();
		}
		
		m_stream << '}';
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

			if (resolveResult.resolved)
			{
				pointerResolved = true;
				Write(resolveResult.resolvedType, resolveResult.resolvedValue);
			}
		}

		if (!pointerResolved)
		{
			// No resolver found, so put null here
			m_stream << k_null;
		}
	}
	else
	{
		// This type of meta-type is not supported?
		m_stream << k_null;
	}
}

void JsonWriter::PrintPadding()
{
	for (int pad = 0; pad < m_padding; ++pad)
	{
		m_stream.put('\t');
	}
}

void JsonWriter::WriteStringLiteral(const char* _literal)
{
	m_stream << k_stringParethesis << _literal << k_stringParethesis;
}

void JsonWriter::WriteStringLiteral(const wchar_t* _literal)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string utf8String = converter.to_bytes(_literal);

	m_stream.put(k_stringParethesis);
	m_stream.write(utf8String.data(), utf8String.size());
	m_stream.put(k_stringParethesis);
}

void JsonWriter::AddPointerTypeResolver(const rttr::Type& type, rttr::PointerTypeResolver* resolver)
{
	m_customPointerTypeResolvers.emplace(type.GetTypeIndex(), resolver);
}
