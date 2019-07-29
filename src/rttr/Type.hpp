#pragma once
#include "helper/TypeTraitsExtension.hpp"
#include "raven_serialize.hpp"

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
}

namespace rttr
{

class Property;
class Type;

using MetaTypeInstanceAllocator = std::function<void*()>;
using MetaTypeInstanceDestructor = std::function<void(void*)>;
using PointerTypeIndexResolverFunc = std::function<std::type_index(void*)>;
using SmartPtrValueResolver = std::function<void* (void*)>;
using SmartPtrValueAssignFunc = std::function<void(void*, void*)>;

struct SmartPtrParams
{
	const char* smartptrTypeName;
	SmartPtrValueAssignFunc valueAssignFunc;
};

struct DynamicArrayParams
{
	using ArrayResizeFunc = std::function<void(void*, const std::size_t)>;
	using ArrayGetSizeFunc = std::function<std::size_t(const void*)>;
	using ArrayGetItemFunc = std::function<void*(const void*, const std::size_t)>;

	ArrayResizeFunc resizeFunc;
	ArrayGetSizeFunc getSizeFunc;
	ArrayGetItemFunc getItemFunc;
};

enum class TypeClass
{
	Invalid,

	Integral,
	Real,
	Array,
	Enum,
	Function,
	Pointer,
	SmartPointer,
};

using DebugValueViewer = const void* (*)(const void*);

struct type_data
{
	const TypeClass typeClass;
	const char* name = nullptr;
	const std::size_t id = 0U;
	const std::size_t size = 0U;
	const std::type_index typeIndex;
	bool isIntegral : 1;
	bool isFloat : 1;
	bool isArray : 1;
	bool isEnum : 1;
	bool isClass : 1;
	bool isFunction : 1;
	bool isPointer : 1;
	bool isSmartPointer : 1;
	bool isMemberObjPointer : 1;
	bool isMemberFuncPointer : 1;
	bool isConst : 1;
	bool isSigned : 1;
	bool isString : 1;
	bool isUserDefined : 1;
	std::size_t arrayRank;
	struct ArrayTraits
	{
		bool isStdVector = false;
		bool isStdArray = false;
		bool isSimpleArray = false;
	} arrayTraits;
	SmartPtrParams* smartptrParams = nullptr;
	DynamicArrayParams* dynamicArrayParams = nullptr;
	Type* underlyingType[2];
	std::shared_ptr<std::size_t> arrayExtents;
	std::vector<std::shared_ptr<Property>> properties;
	MetaTypeInstanceAllocator instanceAllocator;
	MetaTypeInstanceDestructor instanceDestructor;
	PointerTypeIndexResolverFunc pointerTypeIndexResolverFunc;
	SmartPtrValueResolver smartPtrValueResolver;
	DebugValueViewer debugValueViewer = nullptr;

	RAVEN_SER_API type_data(const TypeClass typeClass, const char* name
		, const std::size_t id, const std::size_t size, const std::type_index& typeIndex) noexcept;
};

class Type
{
public:
	RAVEN_SER_API Type(type_data* typeData);

	RAVEN_SER_API const char* GetName() const;
	const std::size_t RAVEN_SER_API GetId() const;
	const std::size_t RAVEN_SER_API GetSize() const;
	const bool RAVEN_SER_API IsValid() const;
	const bool RAVEN_SER_API IsIntegral() const;
	const bool RAVEN_SER_API IsFloatingPoint() const;
	const bool RAVEN_SER_API IsArray() const;
	const bool RAVEN_SER_API IsEnum() const;
	const bool RAVEN_SER_API IsClass() const;
	const bool RAVEN_SER_API IsFunction() const;
	const bool RAVEN_SER_API IsPointer() const;
	const bool RAVEN_SER_API IsSmartPointer() const;
	const bool RAVEN_SER_API IsMemberObjectPointer() const;
	const bool RAVEN_SER_API IsMemberFunctionPointer() const;
	const bool RAVEN_SER_API IsConst() const;
	const bool RAVEN_SER_API IsString() const;
	const bool RAVEN_SER_API IsSigned() const;

	// Get type of the array, enum underlying value
	RAVEN_SER_API const Type& GetUnderlyingType(const std::size_t index = 0U) const;
	const std::size_t RAVEN_SER_API GetArrayRank() const;
	const std::size_t RAVEN_SER_API GetArrayExtent(const std::size_t dimension = 0U) const;
	RAVEN_SER_API Property* GetProperty(const std::size_t propertyIdx) const;
	RAVEN_SER_API Property* FindProperty(const std::string& name) const;
	std::size_t RAVEN_SER_API GetPropertiesCount() const;
	RAVEN_SER_API const std::type_index& GetTypeIndex() const;

	using ArrayIteratorFunc = std::function<void(const Type&, std::size_t, const void*)>;
	void RAVEN_SER_API IterateArray(const void* value, const ArrayIteratorFunc& f) const;
	std::size_t RAVEN_SER_API GetDynamicArraySize(const void* value) const;
	bool RAVEN_SER_API IsDynamicArray() const;
	void RAVEN_SER_API SetDynamicArraySize(void* value, const std::size_t count) const;
	RAVEN_SER_API void* GetArrayItemValuePtr(void* value, const std::size_t idx) const;

	RAVEN_SER_API void* Instantiate() const;
	void RAVEN_SER_API Destroy(void* object) const;

	RAVEN_SER_API void* GetSmartPtrValue(void* value) const;
	std::type_index RAVEN_SER_API GetPointerTypeIndex(void* value) const;
	RAVEN_SER_API const char* GetSmartPtrTypeName() const;
	void RAVEN_SER_API AssignPointerValue(void* pointer, void* value) const;

	bool RAVEN_SER_API operator==(const Type& other) const;
	bool RAVEN_SER_API operator!=(const Type& other) const;

	template <typename Signature>
	Type& DeclProperty(const char* name, Signature signature)
	{
		using T = typename ExtractClassType<Signature>::type;

		auto property = std::make_shared<MemberProperty<T, typename ExtractValueType<Signature>::type, Signature>>(name, signature);
		std::shared_ptr<Property> baseProperty(std::move(property));

		AddProperty(std::move(baseProperty));

		return *this;
	}

	template <typename GetterSignature, typename SetterSignature>
	Type& DeclProperty(const char* name, GetterSignature getter, SetterSignature setter)
	{
		static_assert(std::is_same<ExtractValueType<GetterSignature>::type, ExtractValueType<SetterSignature>::type>::value, "Setter ang getter types mismatch!");

		using T = typename ExtractClassType<GetterSignature>::type;

		auto property = std::make_shared<IndirectProperty<T, typename ExtractValueType<GetterSignature>::type, GetterSignature, SetterSignature>>(name, getter, setter);
		std::shared_ptr<Property> baseProperty(std::move(property));

		AddProperty(std::move(baseProperty));

		return *this;
	}

	RAVEN_SER_API Type& DeclProperty(const char* name, rs::ICustomPropertyResolvePolicy* policy);

	uint64_t RAVEN_SER_API CastToUnsignedInteger(const void* value) const;
	int64_t RAVEN_SER_API CastToSignedInteger(const void* value) const;
	double RAVEN_SER_API CastToFloat(const void* value) const;

	RAVEN_SER_API const void* DebugViewValue(const void* value) const;

private:
	void RAVEN_SER_API AddProperty(std::shared_ptr<Property>&& property);

private:
	type_data* m_typeData = nullptr;
};

} // namespace rttr
