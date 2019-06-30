#include "SerializationContext.hpp"

namespace rs
{
namespace detail
{

DefaultPointerFiller::DefaultPointerFiller(const SerializationContext* context
	, const rttr::Type& type, const std::size_t objectId, void* pointerAddress)
	: IPointerFiller(context)
	, m_type(type)
	, m_objectId(objectId)
	, m_pointerAddress(pointerAddress)
{}

void DefaultPointerFiller::Fill()
{
	const SerializationContext::ObjectData* objectData = m_context->GetObjectById(m_objectId);
	if (nullptr != objectData)
	{
		m_type.AssignPointerValue(m_pointerAddress, const_cast<void*>(objectData->objectPtr));
	}
	else
	{
		m_type.AssignPointerValue(m_pointerAddress, nullptr);
	}
}

PropertyPointerFiller::PropertyPointerFiller(const SerializationContext* context
	, const std::size_t objectId, void* object, const rttr::Property* property)
	: IPointerFiller(context)
	, m_objectId(objectId)
	, m_object(object)
	, m_property(property)
{}

void PropertyPointerFiller::Fill()
{
	const rttr::Type& propertyType = m_property->GetType();
	void* tempPointer = propertyType.Instantiate();

	const SerializationContext::ObjectData* objectData = m_context->GetObjectById(m_objectId);
	if (nullptr != objectData)
	{
		propertyType.AssignPointerValue(tempPointer, const_cast<void*>(objectData->objectPtr));
	}
	else
	{
		propertyType.AssignPointerValue(tempPointer, nullptr);
	}

	m_property->CallMutator(m_object, tempPointer);

	propertyType.Destroy(tempPointer);
}

SerializationContext::ObjectData::ObjectData(const rttr::Type& type, const void* objectPtr, const std::size_t objectId) noexcept
	: type(type)
	, objectPtr(objectPtr)
	, objectId(objectId)
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

void SerializationContext::AddObject(const std::size_t idx, const rttr::Type& type, const void* objectPtr)
{
	m_objects.emplace_back(type, objectPtr, idx);
}

void SerializationContext::AddPointerFiller(std::unique_ptr<IPointerFiller>&& pointerFiller)
{
	m_pointerFillers.push_back(std::move(pointerFiller));
}

const SerializationContext::ObjectData* SerializationContext::GetObjectById(const std::size_t id) const
{
	auto predicate = [id](const ObjectData& data)
	{
		return data.objectId == id;
	};

	auto it = std::find_if(m_objects.begin(), m_objects.end(), predicate);
	if (it != m_objects.end())
	{
		const SerializationContext::ObjectData& data = *it;
		return &data;
	}

	return nullptr;
}

const std::vector<SerializationContext::ObjectData>& SerializationContext::GetObjects() const
{
	return m_objects;
}

const std::vector<std::unique_ptr<IPointerFiller>>& SerializationContext::GetPointerFillers() const
{
	return m_pointerFillers;
}

} // namespace detail
} // namespace rs
