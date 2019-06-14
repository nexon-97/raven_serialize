#include "rttr/Type.hpp"
#include "rttr/Property.hpp"

namespace rttr
{

type_data::type_data(const char* name, const std::size_t id, const std::size_t size, const std::type_index& typeIndex) noexcept
	: name(name)
	, id(id)
	, size(size)
	, typeIndex(typeIndex)
	, underlyingType{ nullptr }
{}

/*type_data::type_data(type_data&& other) noexcept
	: name(name)
	, id(id)
	, properties(std::move(other.properties))
	, arrayExtents(std::move(other.arrayExtents))
	, arrayRank(other.arrayRank)
	, isIntegral(other.isIntegral)
	, isFloat(other.isFloat)
	, isArray(other.isArray)
	, isEnum(other.isEnum)
	, isClass(other.isClass)
	, isFunction(other.isFunction)
	, isPointer(other.isPointer)
	, isMemberObjPointer(other.isMemberObjPointer)
	, isMemberFuncPointer(other.isMemberFuncPointer)
	, isConst(other.isConst)
	, isSigned(other.isSigned)

{}*/

Type::Type(type_data* typeData)
	: m_typeData(typeData)
{}

const bool Type::IsValid() const
{
	return nullptr != m_typeData;
}

const char* Type::GetName() const
{
	return m_typeData->name;
}

const std::size_t Type::GetId() const
{
	return m_typeData->id;
}

const std::size_t Type::GetSize() const
{
	return m_typeData->size;
}

const bool Type::IsIntegral() const
{
	return m_typeData->isIntegral;
}

const bool Type::IsFloatingPoint() const
{
	return m_typeData->isFloat;
}

const bool Type::IsArray() const
{
	return m_typeData->isArray;
}

const bool Type::IsEnum() const
{
	return m_typeData->isEnum;
}

const bool Type::IsClass() const
{
	return m_typeData->isClass;
}

const bool Type::IsFunction() const
{
	return m_typeData->isFunction;
}

const bool Type::IsPointer() const
{
	return m_typeData->isPointer;
}

const bool Type::IsMemberObjectPointer() const
{
	return m_typeData->isMemberObjPointer;
}

const bool Type::IsMemberFunctionPointer() const
{
	return m_typeData->isMemberFuncPointer;
}

const bool Type::IsConst() const
{
	return m_typeData->isConst;
}

const bool Type::IsSigned() const
{
	return m_typeData->isSigned;
}

const bool Type::IsString() const
{
	return m_typeData->isString;
}

const std::size_t Type::GetArrayRank() const
{
	return m_typeData->arrayRank;
}

const std::size_t Type::GetArrayExtent(const std::size_t dimension) const
{
	return m_typeData->arrayExtents.get()[dimension];
}

Property* Type::GetProperty(const std::size_t propertyIdx) const
{
	assert(IsClass());
	return m_typeData->properties[propertyIdx].get();
}

std::size_t Type::GetPropertiesCount() const
{
	assert(IsClass());
	return m_typeData->properties.size();
}

const std::type_index& Type::GetTypeIndex() const
{
	return m_typeData->typeIndex;
}

bool Type::IsDynamicArray() const
{
	return IsArray() && !m_typeData->arrayTraits.isSimpleArray;
}

void Type::AddProperty(std::shared_ptr<Property>&& property)
{
	auto predicate = [&property](const std::shared_ptr<Property>& item)
	{
		return item->GetName() == property->GetName();
	};

	auto it = std::find_if(m_typeData->properties.begin(), m_typeData->properties.end(), predicate);
	assert(it == m_typeData->properties.end());

	m_typeData->properties.emplace_back(std::move(property));
}

uint64_t Type::CastToUnsignedInteger(const void* valuePtr) const
{
	assert(IsIntegral());

	switch (m_typeData->size)
	{
		case 1:
			return static_cast<uint64_t>(*static_cast<const uint8_t*>(valuePtr));
		case 2:
			return static_cast<uint64_t>(*static_cast<const uint16_t*>(valuePtr));
		case 4:
			return static_cast<uint64_t>(*static_cast<const uint32_t*>(valuePtr));
		case 8:
		default:
			return *static_cast<const uint64_t*>(valuePtr);
	}
}

int64_t Type::CastToSignedInteger(const void* valuePtr) const
{
	assert(IsIntegral());

	switch (m_typeData->size)
	{
		case 1:
			return static_cast<int64_t>(*static_cast<const int8_t*>(valuePtr));
		case 2:
			return static_cast<int64_t>(*static_cast<const int16_t*>(valuePtr));
		case 4:
			return static_cast<int64_t>(*static_cast<const int32_t*>(valuePtr));
		case 8:
		default:
			return *static_cast<const int64_t*>(valuePtr);
	}
}

double Type::CastToFloat(const void* valuePtr) const
{
	assert(IsFloatingPoint());
	double value = 0.0;

	if (m_typeData->typeIndex == typeid(float))
	{
		value = static_cast<double>(*static_cast<const float*>(valuePtr));
	}
	else
	{
		value = *static_cast<const double*>(valuePtr);
	}

	return value;
}

void Type::IterateArray(const void* value, const ArrayIteratorFunc& f) const
{
	assert(IsArray());

	if (m_typeData->arrayTraits.isStdArray)
	{
		auto vectorPtr = static_cast<const std::vector<int>*>(value);
		std::size_t size = vectorPtr->size();
		const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(vectorPtr->data());
		auto underlyingType = GetUnderlyingType();

		for (std::size_t i = 0U; i < size; ++i)
		{
			f(underlyingType, i, dataPtr + underlyingType.GetSize() * i);
		}
	}
	else if (m_typeData->arrayTraits.isSimpleArray)
	{
		auto dataPtr = static_cast<const uint8_t*>(value);
		auto underlyingType = GetUnderlyingType();

		for (std::size_t i = 0U; i < m_typeData->arrayExtents.get()[0]; ++i)
		{
			f(underlyingType, i, dataPtr + underlyingType.GetSize() * i);
		}
	}
}

const Type& Type::GetUnderlyingType(const std::size_t index) const
{
	return *m_typeData->underlyingType[index];
}

std::size_t Type::GetDynamicArraySize(const void* value) const
{
	assert(IsArray());

	if (m_typeData->arrayTraits.isStdArray)
	{
		auto vectorPtr = static_cast<const std::vector<int>*>(value);
		std::size_t size = vectorPtr->size();
		
		return size;
	}

	return 0U;
}

void Type::SetDynamicArraySize(void* value, const std::size_t count) const
{
	assert(IsArray());

	if (m_typeData->arrayTraits.isStdArray)
	{
		auto vectorPtr = static_cast<std::vector<float>*>(value);
		vectorPtr->resize(count);
	}
}

void* Type::GetArrayItemValuePtr(void* value, const std::size_t idx) const
{
	assert(IsArray());

	if (m_typeData->arrayTraits.isStdArray)
	{
		auto vectorPtr = static_cast<std::vector<float>*>(value);
		auto underlyingType = GetUnderlyingType();
		return reinterpret_cast<uint8_t*>(vectorPtr->data()) + underlyingType.GetSize() * idx;
	}

	return nullptr;
}

void* Type::Instantiate() const
{
	return std::invoke(m_typeData->instanceAllocator);
}

bool Type::operator==(const Type& other) const
{
	return m_typeData == other.m_typeData;
}

bool Type::operator!=(const Type& other) const
{
	return !(*this == other);
}

} // namespace rttr
