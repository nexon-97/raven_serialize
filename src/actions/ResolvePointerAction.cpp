#include "actions/ResolvePointerAction.hpp"
#include "SerializationContext.hpp"

namespace rs
{
namespace detail
{

ResolvePointerAction::ResolvePointerAction(const std::size_t depth, SerializationContext* context
	, const rttr::Type& pointerType, void* pointerAddress, const std::size_t markerId)
	: IReaderAction(depth)
	, m_context(context)
	, m_pointerType(pointerType)
	, m_pointerAddress(pointerAddress)
	, m_markerId(markerId)
{}

void ResolvePointerAction::Perform()
{
	const SerializationContext::ObjectData* objectData = m_context->GetObjectById(m_markerId);
	if (nullptr != objectData)
	{
		m_pointerType.AssignPointerValue(m_pointerAddress, const_cast<void*>(objectData->objectPtr));
	}
	else
	{
		m_pointerType.AssignPointerValue(m_pointerAddress, nullptr);
	}
}

const ReaderActionType ResolvePointerAction::GetActionType() const
{
	return ReaderActionType::ResolvePtr;
}

} // namespace detail
} // namespace rs
