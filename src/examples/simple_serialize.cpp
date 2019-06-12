#include "Serializer.hpp"
#include "rttr/Property.hpp"
#include "rttr/Class.hpp"
#include "rttr/Manager.hpp"

#include <iostream>
#include <type_traits>
#include <fstream>

class JsonWriter
{
public:
	explicit JsonWriter(std::ostream& stream)
		: m_stream(stream)
		, k_primitiveWriters{
			{ typeid(int), &JsonWriter::WriteInt },
			{ typeid(float), &JsonWriter::WriteFloat },
		}
	{}

	void WriteInt(void* value)
	{
		m_stream << *static_cast<int*>(value);
	}

	void WriteFloat(void* value)
	{
		m_stream << *static_cast<float*>(value);
	}

	void Write(const std::type_index& valueTypeIndex, void* value)
	{
		rttr::ClassBase* metaclass = rttr::Manager::GetRTTRManager().GetClass(valueTypeIndex);

		if (nullptr == metaclass)
		{
			auto it = k_primitiveWriters.find(valueTypeIndex);
			if (it != k_primitiveWriters.end())
			{
				std::invoke(it->second, this, value);
			}
			else
			{
				m_stream << "null";
			}
			
			return;
		}

		m_stream << '{' << std::endl;
		++m_padding;

		const std::size_t propertiesCount = metaclass->GetProperties().size();
		std::size_t i = 0U;
		for (const auto& propertyData : metaclass->GetProperties())
		{
			for (int pad = 0; pad < m_padding; ++pad)
			{
				m_stream.put('\t');
			}

			m_stream << '"' << propertyData.first << "\" : ";

			auto concreteProperty = propertyData.second.get();
			void* propertyValuePtr = concreteProperty->GetValue(value);

			Write(concreteProperty->GetValueTypeIndex(), propertyValuePtr);

			if (i + 1U < propertiesCount)
			{
				m_stream.put(',');
			}

			m_stream << std::endl;
			++i;
		}

		--m_padding;

		for (int pad = 0; pad < m_padding; ++pad)
		{
			m_stream.put('\t');
		}
		m_stream << '}';
	}

private:
	std::ostream& m_stream;
	const std::unordered_map<std::type_index, void(JsonWriter::*)(void*)> k_primitiveWriters;
	int m_padding = 0;
};

struct Vector3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
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

	rttr::DeclClass<TestStruct>("TestStruct")
		.DeclProperty("a", &TestStruct::GetA, &TestStruct::SetA)
		.DeclProperty("position", &TestStruct::position)
		.DeclProperty("scale", &TestStruct::scale)
		.DeclProperty("rotation", &TestStruct::rotation);

	rttr::DeclClass<Vector3>("Vector3")
		.DeclProperty("x", &Vector3::x)
		.DeclProperty("y", &Vector3::y)
		.DeclProperty("z", &Vector3::z);

	rttr::DeclClass<Quaternion>("Quaternion")
		.DeclProperty("x", &Quaternion::x)
		.DeclProperty("y", &Quaternion::y)
		.DeclProperty("z", &Quaternion::z)
		.DeclProperty("w", &Quaternion::w);

	serializer.Write(typeid(TestStruct), &value);

	return 0;
}
