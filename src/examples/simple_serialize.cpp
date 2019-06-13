#include "Serializer.hpp"
#include "rttr/Property.hpp"
#include "rttr/Class.hpp"
#include "rttr/Manager.hpp"

#include <iostream>
#include <type_traits>
#include <fstream>

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
				std::size_t arraySize = type.GetDynamicArraySize(&value);

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

int main()
{
	std::ofstream stream("test.json");
	raven::Serializer<JsonWriter> serializer(stream);

	TestStruct value;
	value.a = 15;
	value.position = { 5.f, 15.f, 0.5f };
	value.scale = { 1.f, 1.f, 1.f };
	value.timestamp = "It's OK, dude!";
	value.scale.someBoolVar = false;
	value.vec = { 5, 7, 25, -15, 0 };
	value.somevec[0] = 25;
	value.somevec[6] = -92;

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
		.DeclProperty("somevec", &TestStruct::somevec);

	serializer.Write(value);

	return 0;
}
