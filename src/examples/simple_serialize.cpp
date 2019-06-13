#include "Serializer.hpp"
#include "Deserializer.hpp"

#include "rttr/Property.hpp"
#include "rttr/Manager.hpp"

#include "writers/JsonWriter.hpp"
#include "writers/JsonReader.hpp"

#include <fstream>

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
