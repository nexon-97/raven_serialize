#include "rttr/Property.hpp"
#include "rttr/Manager.hpp"

#include <fstream>

struct Vector3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	
	Vector3() = default;
	Vector3(float value)
		: x(value)
		, y(value)
		, z(value)
	{}

	Vector3(float x, float y, float z)
		: x(x)
		, y(y)
		, z(z)
	{}
};

struct Quaternion
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;

	Quaternion()
		: x(0.f)
		, y(0.f)
		, z(0.f)
		, w(1.f)
	{}

	Quaternion(float x, float y, float z, float w)
		: x(x)
		, y(y)
		, z(z)
		, w(w)
	{}
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

	TestStruct(int a, const Vector3& v1, const Quaternion& q, const std::string& timestamp)
		: a(a)
		, position(v1)
		, rotation(q)
		, timestamp(timestamp)
		, scale(Vector3(1.f))
	{}

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

	rttr::DeclType<Vector3>("Vector3")
		//.DeclConstructor<Vector3>()
		//.DeclConstructor<Vector3, float>()
		//.DeclConstructor<Vector3, float, float, float>()
		.DeclProperty("x", &Vector3::x)
		.DeclProperty("y", &Vector3::y)
		.DeclProperty("z", &Vector3::z)
		;

	rttr::DeclType<Quaternion>("Quaternion")
		//.DeclConstructor<Quaternion>()
		//.DeclConstructor<Quaternion, float, float, float, float>()
		.DeclProperty("x", &Quaternion::x)
		.DeclProperty("y", &Quaternion::y)
		.DeclProperty("z", &Quaternion::z)
		.DeclProperty("w", &Quaternion::w);

	rttr::DeclType<TestStruct>("TestStruct")
		.DeclProperty("a", &TestStruct::GetA, &TestStruct::SetA)
		.DeclProperty("position", &TestStruct::position)
		.DeclProperty("scale", &TestStruct::scale)
		.DeclProperty("rotation", &TestStruct::rotation)
		.DeclProperty("timestamp", &TestStruct::timestamp)
		.DeclProperty("vec", &TestStruct::vec)
		;

	return 0;
}
