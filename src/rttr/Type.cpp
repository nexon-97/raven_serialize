#include "rttr/Type.hpp"
#include "rttr/Property.hpp"
#include "rttr/Manager.hpp"
#include "rttr/details/ObjectClassParams.hpp"
#include "rttr/details/PointerParams.hpp"
#include "rttr/details/ArrayParams.hpp"
#include "rttr/details/ScalarParams.hpp"
#include "rttr/details/EnumParams.hpp"
#include "rs/ICustomPropertyResolvePolicy.hpp"

namespace rttr
{

type_data::type_data(const TypeClass typeClass, const char* name
	, const std::size_t id, const std::size_t size, const std::type_index& typeIndex) noexcept
	: name(name)
	, id(id)
	, size(size)
	, typeIndex(typeIndex)
	, isUserDefined(false)
	, typeClass(typeClass)
{}

type_data::type_data(type_data&& other)
	: typeClass(other.typeClass)
	, name(other.name)
	, id(other.id)
	, size(other.size)
	, typeIndex(other.typeIndex)
	, isConst(other.isConst)
	, isUserDefined(other.isUserDefined)
	, instanceAllocator(other.instanceAllocator)
	, instanceDestructor(other.instanceDestructor)
	, debugValueViewer(other.debugValueViewer)
{}

Type::Type()
	: m_typeData(nullptr)
{}

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

TypeClass Type::GetTypeClass() const
{
	return m_typeData->typeClass;
}

const bool Type::IsConst() const
{
	return m_typeData->isConst;
}

const std::size_t Type::GetArrayRank() const
{
	assert(m_typeData->typeClass == TypeClass::Array);
	return m_typeData->typeParams.array_->arrayRank;
}

const std::size_t Type::GetArrayExtent(const std::size_t dimension) const
{
	assert(m_typeData->typeClass == TypeClass::Array);
	return m_typeData->typeParams.array_->arrayExtents[dimension];
}

Type Type::GetEnumUnderlyingType() const
{
	assert(m_typeData->typeClass == TypeClass::Enum);
	return m_typeData->typeParams.enum_->underlyingType;
}

Type Type::GetPointedType() const
{
	assert(m_typeData->typeClass == TypeClass::Pointer);
	return m_typeData->typeParams.pointer->pointedType;
}

Property* Type::GetProperty(const std::size_t propertyIdx) const
{
	assert(m_typeData->typeClass == TypeClass::Object);
	return m_typeData->typeParams.object->properties[propertyIdx].get();
}

Property* Type::FindProperty(const std::string& name) const
{
	assert(m_typeData->typeClass == TypeClass::Object);

	for (const auto& property : m_typeData->typeParams.object->properties)
	{
		if (property->GetName() == name)
		{
			return property.get();
		}
	}

	return nullptr;
}

std::size_t Type::GetPropertiesCount() const
{
	assert(m_typeData->typeClass == TypeClass::Object);
	return m_typeData->typeParams.object->properties.size();
}

const std::type_index& Type::GetTypeIndex() const
{
	return m_typeData->typeIndex;
}

void Type::AddProperty(std::unique_ptr<Property>&& property)
{
	assert(m_typeData->typeClass == TypeClass::Object);

	if (!property)
		return;

	auto predicate = [&property](const std::unique_ptr<Property>& item)
	{
		return item->GetName() == property->GetName();
	};

	auto& properties = m_typeData->typeParams.object->properties;
	auto it = std::find_if(properties.begin(), properties.end(), predicate);
	assert(it == properties.end());

	properties.emplace_back(std::move(property));
}

uint64_t Type::CastToUnsignedInteger(const void* valuePtr) const
{
	assert(m_typeData->typeClass == TypeClass::Integral);

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
	assert(m_typeData->typeClass == TypeClass::Integral);

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
	assert(m_typeData->typeClass == TypeClass::Real);
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

bool Type::IsSignedIntegral() const
{
	assert(m_typeData->typeClass == TypeClass::Integral);
	return m_typeData->typeParams.scalar->isSigned;
}

//void Type::IterateArray(const void* value, const ArrayIteratorFunc& f) const
//{
//	//assert(IsArray());
//
//	if (m_typeData->arrayTraits.isStdArray)
//	{
//		std::size_t size = GetDynamicArraySize(value);
//
//		auto vectorPtr = reinterpret_cast<const std::vector<int>*>(value);
//		uintptr_t dataPtr = reinterpret_cast<uintptr_t>(vectorPtr->data());
//		auto underlyingType = GetUnderlyingType();
//		const std::size_t itemTypeSize = underlyingType.GetSize();
//
//		for (std::size_t i = 0U; i < size; ++i)
//		{
//			f(underlyingType, i, reinterpret_cast<void*>(dataPtr));
//			dataPtr += itemTypeSize;
//		}
//	}
//	else if (m_typeData->arrayTraits.isSimpleArray)
//	{
//		auto dataPtr = static_cast<const uint8_t*>(value);
//		auto underlyingType = GetUnderlyingType();
//
//		for (std::size_t i = 0U; i < m_typeData->arrayExtents.get()[0]; ++i)
//		{
//			f(underlyingType, i, dataPtr + underlyingType.GetSize() * i);
//		}
//	}
//}

//std::size_t Type::GetDynamicArraySize(const void* value) const
//{
//	//assert(IsArray());
//	return m_typeData->dynamicArrayParams->getSizeFunc(value);
//}
//
//void Type::SetDynamicArraySize(void* value, const std::size_t count) const
//{
//	//assert(IsArray());
//	m_typeData->dynamicArrayParams->resizeFunc(value, count);
//}
//
//void* Type::GetArrayItemValuePtr(void* value, const std::size_t idx) const
//{
//	//assert(IsArray());
//	return m_typeData->dynamicArrayParams->getItemFunc(value, idx);
//}

void* Type::Instantiate() const
{
	return std::invoke(m_typeData->instanceAllocator);
}

void Type::Destroy(void* object) const
{
	std::invoke(m_typeData->instanceDestructor, object);
}

//void* Type::GetSmartPtrValue(void* value) const
//{
//	assert(IsSmartPointer());
//
//	if (m_typeData->smartPtrValueResolver)
//	{
//		return m_typeData->smartPtrValueResolver(value);
//	}
//	
//	return nullptr;
//}

//const char* Type::GetSmartPtrTypeName() const
//{
//	assert(IsSmartPointer());
//	return m_typeData->smartptrParams->smartptrTypeName;
//}
//
//void Type::AssignPointerValue(void* pointer, void* value) const
//{
//	if (IsSmartPointer())
//	{
//		m_typeData->smartptrParams->valueAssignFunc(pointer, value);
//	}
//}

bool Type::operator==(const Type& other) const
{
	return m_typeData == other.m_typeData;
}

bool Type::operator!=(const Type& other) const
{
	return !(*this == other);
}

const void* Type::DebugViewValue(const void* value) const
{
	return m_typeData->debugValueViewer(value);
}

TypeProxyData* Type::GetProxyType() const
{
	return Manager::GetRTTRManager().GetProxyType(Type(m_typeData));
}

void Type::RegisterProxy(const Type& proxyType)
{
	Manager::GetRTTRManager().RegisterProxyType(Type(m_typeData), proxyType);
}

std::size_t Type::GetHash() const
{
	if (m_typeData)
	{
		return m_typeData->id;
	}

	return 0U;
}

} // namespace rttr
