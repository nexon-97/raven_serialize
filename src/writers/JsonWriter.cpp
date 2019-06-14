#include "writers/JsonWriter.hpp"
#include "rttr/Property.hpp"

namespace
{

const char* k_true = "true";
const char* k_false = "false";
const char* k_null = "null";
const char* k_typeId = "$type$";

}

JsonWriter::JsonWriter(std::ostream& stream, const bool writeClassNames)
	: m_stream(stream)
	, m_writeClassNames(writeClassNames)
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
		m_stream << '"';

		if (type.GetTypeIndex() == typeid(std::string))
		{
			m_stream << *static_cast<const std::string*>(value);
		}
		else if (type.GetTypeIndex() == typeid(const char*))
		{
			m_stream << *static_cast<const char*>(value);
		}

		m_stream << '"';
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
	else if (type.IsFloatingPoint())
	{
		double floatValue = type.CastToFloat(value);
		m_stream << floatValue;
	}
	else if (type.IsArray())
	{
		m_stream << '[' << std::endl;
		++m_padding;

		if (type.IsDynamicArray())
		{
			std::size_t arraySize = type.GetDynamicArraySize(value);

			auto f = [this, arraySize](const rttr::Type& itemType, std::size_t index, const void* itemPtr)
			{
				PrintPadding();
				Write(itemType, itemPtr);

				if (index + 1U < arraySize)
				{
					m_stream.put(',');

				}

				m_stream << std::endl;
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
					PrintPadding();
					Write(itemType, itemPtr);

					if (index + 1U < arraySize)
					{
						m_stream.put(',');

					}

					m_stream << std::endl;
				};

				type.IterateArray(value, f);
			}
		}

		--m_padding;
		PrintPadding();
		m_stream << ']';
	}
	else if (type.IsClass())
	{
		m_stream << '{' << std::endl;
		++m_padding;

		std::size_t propertiesCount = type.GetPropertiesCount();

		if (m_writeClassNames)
		{
			PrintPadding();
			m_stream << '"' << k_typeId << "\" : \"" << type.GetName() << '"';

			if (propertiesCount > 0U)
			{
				m_stream << ',';
			}
			m_stream << std::endl;
		}

		for (std::size_t i = 0U; i < propertiesCount; ++i)
		{
			auto property = type.GetProperty(i);
			const rttr::Type& propertyType = property->GetType();

			PrintPadding();
			m_stream << '"' << property->GetName() << "\" : ";

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

			m_stream << std::endl;
		}

		--m_padding;
		PrintPadding();
		m_stream << '}';
	}
}

void JsonWriter::PrintPadding()
{
	for (int pad = 0; pad < m_padding; ++pad)
	{
		m_stream.put('\t');
	}
}
