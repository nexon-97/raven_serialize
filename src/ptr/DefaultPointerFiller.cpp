#include "ptr/DefaultPointerFiller.hpp"
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

} // namespace detail
} // namespace rs
