#pragma once
#include "raven_serialize_export.h"
#include "helper/TypeTraitsExtension.hpp"
#include "rttr/ProxyConverter.hpp"
#include "rttr/TypeClass.hpp"
#include "rttr/details/CollectionInserter.hpp"
#include "rttr/details/CollectionIterator.hpp"
#include "rs/SerializationMethod.hpp"

#include <unordered_map>
#include <string>
#include <memory>
#include <cassert>
#include <vector>
#include <typeindex>
#include <functional>

namespace rs
{
class ICustomPropertyResolvePolicy;
class SerializationAdapter;
}

namespace rttr
{

class Property;
class Type;
struct TypeProxyData;

template <typename T>
Type Reflect();
template <typename Signature>
std::unique_ptr<Property> CreateMemberProperty(const char* name, Signature signature);
template <typename GetterSignature, typename SetterSignature>
std::unique_ptr<Property> CreateIndirectProperty(const char* name, GetterSignature getter, SetterSignature setter);

using MetaTypeInstanceAllocator = std::function<void*()>;
using MetaTypeInstanceDestructor = std::function<void(void*)>;

template <typename ...Args>
std::vector<Type> ReflectArgTypes();

using DebugValueViewer = const void* (*)(const void*);

// Forward declarations of type class params
struct ObjectClassParams;
struct PointerParams;
struct ScalarParams;
struct ArrayParams;
struct EnumParams;

struct type_data
{
	const TypeClass typeClass;
	const char* name = nullptr;
	const std::size_t id = 0U;
	const std::size_t size = 0U;
	const std::type_index typeIndex;
	MetaTypeInstanceAllocator instanceAllocator;
	MetaTypeInstanceDestructor instanceDestructor;
	Type* bases = nullptr;
	uint8_t basesCount = 0U;
	bool isConst : 1;
	bool isUserDefined : 1;
	DebugValueViewer debugValueViewer = nullptr;

	union TypeParams
	{
		ObjectClassParams* object;
		PointerParams* pointer;
		ScalarParams* scalar;
		ArrayParams* array_;
		EnumParams* enum_;
	} typeParams;

	RAVEN_SERIALIZE_API type_data(const TypeClass typeClass, const char* name
		, const std::size_t id, const std::size_t size, const std::type_index& typeIndex) noexcept;

	type_data(const type_data&) = delete;
	type_data& operator=(const type_data&) = delete;
	RAVEN_SERIALIZE_API type_data(type_data&&);
};

class Type
{
public:
	RAVEN_SERIALIZE_API Type();
	RAVEN_SERIALIZE_API Type(type_data* typeData);

	// Main type parameters
	RAVEN_SERIALIZE_API const char* GetName() const;
	TypeClass RAVEN_SERIALIZE_API GetTypeClass() const;
	const std::size_t RAVEN_SERIALIZE_API GetId() const;
	const std::size_t RAVEN_SERIALIZE_API GetSize() const;
	RAVEN_SERIALIZE_API const std::type_index& GetTypeIndex() const;
	std::pair<Type*, uint8_t> RAVEN_SERIALIZE_API GetBaseClasses() const;
	bool RAVEN_SERIALIZE_API IsBaseClass(const Type& other) const;
	void RAVEN_SERIALIZE_API SetSerializationAdapter(std::unique_ptr<rs::SerializationAdapter>&& adapter) const;
	RAVEN_SERIALIZE_API rs::SerializationAdapter* GetSerializationAdapter() const;
	const bool RAVEN_SERIALIZE_API IsConst() const;
	bool RAVEN_SERIALIZE_API IsPolymorphic() const;
	std::size_t RAVEN_SERIALIZE_API GetHash() const;
	rs::SerializationMethod RAVEN_SERIALIZE_API GetSerializationMethod() const;

	const bool RAVEN_SERIALIZE_API IsValid() const;
	RAVEN_SERIALIZE_API operator bool() const;

	// Constructor and destructor
	RAVEN_SERIALIZE_API void* Instantiate() const;
	void RAVEN_SERIALIZE_API Destroy(void* object) const;

	// Object type class interface
	RAVEN_SERIALIZE_API Property* GetProperty(const std::size_t propertyIdx) const;
	RAVEN_SERIALIZE_API Property* FindProperty(const std::string& name) const;
	std::size_t RAVEN_SERIALIZE_API GetPropertiesCount() const;
	void RAVEN_SERIALIZE_API AddProperty(std::unique_ptr<Property>&& property);
	bool RAVEN_SERIALIZE_API IsCollection() const;
	std::unique_ptr<CollectionInserterBase> RAVEN_SERIALIZE_API CreateCollectionInserter(void* collection) const;
	std::unique_ptr<CollectionIteratorBase> RAVEN_SERIALIZE_API CreateCollectionIterator(void* collection) const;
	Type RAVEN_SERIALIZE_API GetCollectionItemType() const;

	// Proxy logic
	void RAVEN_SERIALIZE_API RegisterProxy(const Type& proxyType);
	RAVEN_SERIALIZE_API TypeProxyData* GetProxyType() const;

	// Scalar type interface
	uint64_t RAVEN_SERIALIZE_API CastToUnsignedInteger(const void* value) const;
	int64_t RAVEN_SERIALIZE_API CastToSignedInteger(const void* value) const;
	double RAVEN_SERIALIZE_API CastToFloat(const void* value) const;
	bool RAVEN_SERIALIZE_API IsSignedIntegral() const;

	// Enum type interface
	Type RAVEN_SERIALIZE_API GetEnumUnderlyingType() const;

	// Pointer type interface
	Type RAVEN_SERIALIZE_API GetPointedType() const;

	// Array type interface
	const std::size_t RAVEN_SERIALIZE_API GetArrayRank() const;
	const std::size_t RAVEN_SERIALIZE_API GetArrayExtent(const std::size_t dimension = 0U) const;
	Type RAVEN_SERIALIZE_API GetArrayType() const;

	void RAVEN_SERIALIZE_API SetBaseClasses(Type* types, uint8_t count);

	bool RAVEN_SERIALIZE_API operator==(const Type& other) const;
	bool RAVEN_SERIALIZE_API operator!=(const Type& other) const;

	RAVEN_SERIALIZE_API const void* DebugViewValue(const void* value) const;

private:
	type_data* m_typeData = nullptr;
};

template <typename ...Args>
std::vector<Type> ReflectArgTypes()
{
	std::vector<Type> argTypes;
	(argTypes.push_back(Reflect<Args>()), ...);

	return argTypes;
}

} // namespace rttr

// Declare std::hash specialization
namespace std
{

template <>
struct hash<rttr::Type>
{
	std::size_t operator()(const rttr::Type& type) const
	{
		return type.GetHash();
	}
};

}
