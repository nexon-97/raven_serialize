#include "Serializer.hpp"
#include "Deserializer.hpp"

#include "rttr/Property.hpp"
#include "rttr/Class.hpp"
#include "rttr/Manager.hpp"

#include <iostream>
#include <type_traits>
#include <fstream>

#include <json/json.h>

const char* k_true = "true";
const char* k_false = "false";
const char* k_null = "null";

struct TestStruct;

class JsonWriter
{
public:
	explicit JsonWriter(std::ostream& stream)
		: m_stream(stream)
	{}

	template <typename T>
	void Write(const T& value)
	{
		Write(rttr::Reflect<T>(), &value);
	}

	void Write(const rttr::Type& type, const void* value)
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

private:
	void PrintPadding()
	{
		for (int pad = 0; pad < m_padding; ++pad)
		{
			m_stream.put('\t');
		}
	}

private:
	std::ostream& m_stream;
	int m_padding = 0;
};

////////////////////////////////////////////////////////////////////////

class JsonReader
{
public:
	explicit JsonReader(std::istream& stream)
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

	template <typename T>
	void Read(T& value)
	{
		Read(rttr::Reflect<T>(), &value, m_jsonRoot);
	}

	void Read(const rttr::Type& type, void* value, const Json::Value& jsonVal)
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

private:
	std::istream& m_stream;
	Json::Value m_jsonRoot;
};

////////////////////////////////////////////////////////////////////////

struct Vector3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	bool someBoolVar = true;
};

struct Quaternion
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;
};

struct TestStruct
{
	int a = 0;
	Vector3 position;
	Vector3 scale;
	Quaternion rotation;
	std::string timestamp;
	std::vector<int> vec;
	int somevec[15];

	void SetA(const int value)
	{
		a = value;
	}

	int GetA() const
	{
		return a;
	}
};

////////////////////////////////////////////////////////////////////////

int main()
{
	/*std::ofstream stream("test.json");
	raven::Serializer<JsonWriter> serializer(stream);

	TestStruct value;
	value.a = 15;
	value.position = { 5.f, 15.f, 0.5f };
	value.scale = { 1.f, 1.f, 1.f };
	value.timestamp = "It's OK, dude!";
	value.scale.someBoolVar = false;
	value.vec = { 5, 7, 25, -15, 0 };
	value.somevec[0] = 25;
	value.somevec[6] = -92;*/

	rttr::MetaType<Vector3>("Vector3")
		.DeclProperty("x", &Vector3::x)
		.DeclProperty("y", &Vector3::y)
		.DeclProperty("z", &Vector3::z)
		.DeclProperty("someBoolVar", &Vector3::someBoolVar);

	rttr::MetaType<Quaternion>("Quaternion")
		.DeclProperty("x", &Quaternion::x)
		.DeclProperty("y", &Quaternion::y)
		.DeclProperty("z", &Quaternion::z)
		.DeclProperty("w", &Quaternion::w);

	rttr::MetaType<TestStruct>("TestStruct")
		.DeclProperty("a", &TestStruct::GetA, &TestStruct::SetA)
		.DeclProperty("position", &TestStruct::position)
		.DeclProperty("scale", &TestStruct::scale)
		.DeclProperty("rotation", &TestStruct::rotation)
		.DeclProperty("timestamp", &TestStruct::timestamp)
		.DeclProperty("vec", &TestStruct::vec)
		//.DeclProperty("somevec", &TestStruct::somevec)
		;

	/*serializer.Write(value);
	stream.flush();
	stream.close();*/

	std::ifstream istream("test.json");
	raven::Deserializer<JsonReader> deserializer(istream);

	TestStruct readValue;
	deserializer.Read(readValue);

	return 0;
}
