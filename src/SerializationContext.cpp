#include "SerializationContext.hpp"

namespace rs
{
namespace detail
{

SerializationContext::ObjectData::ObjectData(const rttr::Type& type, const void* objectPtr, const std::size_t objectId) noexcept
	: type(type)
	, objectPtr(objectPtr)
	, objectId(objectId)
{}

SerializationContext::PointerToFillData::PointerToFillData(const rttr::Type& type, const std::size_t objectId, void* pointerAddress) noexcept
	: type(type)
	, objectId(objectId)
	, pointerAddress(pointerAddress)
{}

std::size_t SerializationContext::AddObject(const rttr::Type& type, const void* objectPtr)
{
	auto predicate = [&type, objectPtr](const ObjectData& data)
	{
		return data.objectPtr == objectPtr && data.type == type;
	};

	// Find the same object in context
	auto it = std::find_if(m_objects.begin(), m_objects.end(), predicate);
	if (it == m_objects.end())
	{
		// Ojbect not found, create new entry
		std::size_t objectId = m_objects.size();
		m_objects.emplace_back(type, objectPtr, objectId);

		return objectId;
	}
	else
	{
		// Object found, return its id
		return it->objectId;
	}
}

void SerializationContext::AddPointerToFill(const rttr::Type& type, const std::size_t objectId, void* pointerAddress)
{
	m_pointersToFill.emplace_back(type, objectId, pointerAddress);
}

SerializationContext::ObjectData* SerializationContext::GetObjectById(const std::size_t id)
{
	auto predicate = [id](const ObjectData& data)
	{
		return data.objectId == id;
	};

	auto it = std::find_if(m_objects.begin(), m_objects.end(), predicate);
	if (it != m_objects.end())
	{
		SerializationContext::ObjectData& data = *it;
		return &data;
	}

	return nullptr;
}

const std::vector<SerializationContext::ObjectData>& SerializationContext::GetObjects() const
{
	return m_objects;
}

const std::vector<SerializationContext::PointerToFillData>& SerializationContext::GetPointersToFill() const
{
	return m_pointersToFill;
}

} // namespace detail
} // namespace rs
