#include "actions/ResolvePointerAction.hpp"
#include "SerializationContext.hpp"
#include "rs/log/Log.hpp"

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
	auto pointerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_pointerAddress));

	const SerializationContext::ObjectData* objectData = m_context->GetObjectById(m_markerId);
	if (nullptr != objectData)
	{
		auto pointerValueAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(objectData->objectPtr));
		Log::LogMessage("ResolvePointerAction performed. Pointer at 0x%llX filled with address 0x%llX", pointerAsInt, pointerValueAsInt);

		m_pointerType.AssignPointerValue(m_pointerAddress, const_cast<void*>(objectData->objectPtr));
	}
	else
	{
		Log::LogMessage("ResolvePointerAction performed. Pointer at 0x%llX filled with address null", pointerAsInt);
		m_pointerType.AssignPointerValue(m_pointerAddress, nullptr);
	}
}

const ReaderActionType ResolvePointerAction::GetActionType() const
{
	return ReaderActionType::ResolvePtr;
}

} // namespace detail
} // namespace rs
