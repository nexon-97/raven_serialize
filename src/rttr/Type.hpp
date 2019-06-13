#pragma once
#include "helper/TypeTraitsExtension.hpp"

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

	type_data(const char* name, const std::size_t id, const std::size_t size, const std::type_index& typeIndex) noexcept;
};

class Type
{
public:
	Type(type_data* typeData);

	const char* GetName() const;
	const std::size_t GetId() const;
	const std::size_t GetSize() const;
	const bool IsIntegral() const;
	const bool IsFloatingPoint() const;
	const bool IsArray() const;
	const bool IsEnum() const;
	const bool IsClass() const;
	const bool IsFunction() const;
	const bool IsPointer() const;
	const bool IsMemberObjectPointer() const;
	const bool IsMemberFunctionPointer() const;
	const bool IsConst() const;
	const bool IsString() const;
	const bool IsSigned() const;
	// Get type of the array, enum underlying value
	const Type& GetUnderlyingType(const std::size_t index = 0U) const;
	const std::size_t GetArrayRank() const;
	const std::size_t GetArrayExtent(const std::size_t dimension = 0U) const;
	Property* Type::GetProperty(const std::size_t propertyIdx) const;
	std::size_t GetPropertiesCount() const;
	const std::type_index& GetTypeIndex() const;

	using ArrayIteratorFunc = std::function<void(const Type&, std::size_t, const void*)>;
	void IterateArray(const void* value, const ArrayIteratorFunc& f) const;
	std::size_t GetDynamicArraySize(const void* value) const;

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

	uint64_t CastToUnsignedInteger(const void* value) const;
	int64_t CastToSignedInteger(const void* value) const;
	double CastToFloat(const void* value) const;

private:
	void AddProperty(std::shared_ptr<Property>&& property);

private:
	type_data* m_typeData = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T CastValue(const Type& type, const T* object)
{
	/*if (std::is_integral<T>::value)
	{
		assert(type.IsIntegral());
		int64_t context = 0;
		std::memcpy(&context, object, type.GetSize());

		return static_cast<T>(context);
	}
	else if (is_string<T>::value)
	{
		assert(type.IsString());
		std::string context = *reinterpret_cast<const std::string*>(object);

		//return static_cast<T>(context);
	}*/

	assert(false);
	return T();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace rttr
