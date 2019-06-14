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

namespace rttr
{

class Property;
class Type;

using MetaTypeInstanceAllocator = std::function<void*()>;

struct type_data
{
	const char* name = nullptr;
	const std::size_t id = 0U;
	const std::size_t size = 0U;
	const std::type_index typeIndex;
	bool isIntegral;
	bool isFloat;
	bool isArray;
	bool isEnum;
	bool isClass;
	bool isFunction;
	bool isPointer;
	bool isMemberObjPointer;
	bool isMemberFuncPointer;
	bool isConst;
	bool isSigned;
	bool isString;
	std::size_t arrayRank;
	struct ArrayTraits
	{
		bool isStdVector = false;
		bool isStdArray = false;
		bool isSimpleArray = false;
	} arrayTraits;
	Type* underlyingType[2];
	std::shared_ptr<std::size_t> arrayExtents;
	std::vector<std::shared_ptr<Property>> properties;
	MetaTypeInstanceAllocator instanceAllocator;

	RAVEN_SER_API type_data(const char* name, const std::size_t id, const std::size_t size, const std::type_index& typeIndex) noexcept;
};

class Type
{
public:
	RAVEN_SER_API Type(type_data* typeData);

	RAVEN_SER_API const char* GetName() const;
	const std::size_t RAVEN_SER_API GetId() const;
	const std::size_t RAVEN_SER_API GetSize() const;
	const bool RAVEN_SER_API IsIntegral() const;
	const bool RAVEN_SER_API IsFloatingPoint() const;
	const bool RAVEN_SER_API IsArray() const;
	const bool RAVEN_SER_API IsEnum() const;
	const bool RAVEN_SER_API IsClass() const;
	const bool RAVEN_SER_API IsFunction() const;
	const bool RAVEN_SER_API IsPointer() const;
	const bool RAVEN_SER_API IsMemberObjectPointer() const;
	const bool RAVEN_SER_API IsMemberFunctionPointer() const;
	const bool RAVEN_SER_API IsConst() const;
	const bool RAVEN_SER_API IsString() const;
	const bool RAVEN_SER_API IsSigned() const;
	// Get type of the array, enum underlying value
	RAVEN_SER_API const Type& GetUnderlyingType(const std::size_t index = 0U) const;
	const std::size_t RAVEN_SER_API GetArrayRank() const;
	const std::size_t RAVEN_SER_API GetArrayExtent(const std::size_t dimension = 0U) const;
	RAVEN_SER_API Property* Type::GetProperty(const std::size_t propertyIdx) const;
	std::size_t RAVEN_SER_API GetPropertiesCount() const;
	RAVEN_SER_API const std::type_index& GetTypeIndex() const;

	using ArrayIteratorFunc = std::function<void(const Type&, std::size_t, const void*)>;
	void RAVEN_SER_API IterateArray(const void* value, const ArrayIteratorFunc& f) const;
	std::size_t RAVEN_SER_API GetDynamicArraySize(const void* value) const;
	bool RAVEN_SER_API IsDynamicArray() const;
	void RAVEN_SER_API SetDynamicArraySize(void* value, const std::size_t count) const;
	RAVEN_SER_API void* GetArrayItemValuePtr(void* value, const std::size_t idx) const;

	RAVEN_SER_API void* Instantiate() const;

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

	uint64_t RAVEN_SER_API CastToUnsignedInteger(const void* value) const;
	int64_t RAVEN_SER_API CastToSignedInteger(const void* value) const;
	double RAVEN_SER_API CastToFloat(const void* value) const;

private:
	void RAVEN_SER_API AddProperty(std::shared_ptr<Property>&& property);

private:
	type_data* m_typeData = nullptr;
};

} // namespace rttr
