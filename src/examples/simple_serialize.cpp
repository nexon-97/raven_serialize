#include "Serializer.hpp"
#include "rttr/Property.hpp"
#include "rttr/Class.hpp"

#include <iostream>
#include <type_traits>
#include <fstream>

class JsonWriter
{
public:
	explicit JsonWriter(std::ostream& stream)
		: m_stream(stream)
	{}

	template <typename T>
	void Write(const T& value)
	{
		
	}

private:
	std::ostream& m_stream;
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
};

template <typename T, typename ValueType>
rttr::Property<T, ValueType, rttr::MemberSignature<T, ValueType>> ConstructProperty(const std::string& name, rttr::MemberSignature<T, ValueType> signature)
{
	return rttr::Property<T, ValueType, rttr::MemberSignature<T, ValueType>>(name, signature);
}

int main()
{
	std::ofstream stream("test.json");
	raven::Serializer<JsonWriter> serializer(stream);

	auto propA = ConstructProperty("a", &TestStruct::a);
	auto propPosition = ConstructProperty("position", &TestStruct::position);
	auto propScale = ConstructProperty("scale", &TestStruct::scale);

	TestStruct value;
	value.a = 15;
	value.position = { 5.f, 15.f, 0.5f };
	value.scale = { 1.f, 1.f, 1.f };

	auto testStructDef = rttr::Class<TestStruct>("TestStruct")
		/*.DeclProperty("a", &TestStruct::a)*/;

	propA.SetValue(&value, 25);
	propPosition.SetValue(&value, { 10.f, 15.f, 1.f });
	propScale.SetValue(&value, { 1.f, 2.f, 1.f });

	serializer.Write(value);

	return 0;
}
