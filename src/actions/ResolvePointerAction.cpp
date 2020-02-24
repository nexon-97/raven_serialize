#include "actions/ResolvePointerAction.hpp"
#include "SerializationContext.hpp"
#include "rs/log/Log.hpp"
#include "rttr/Manager.hpp"

namespace rs
{
namespace detail
{

ResolvePointerAction::ResolvePointerAction(const std::size_t depth, SerializationContext* context
	, void* pointerAddress, const uint64_t objectId)
	: IReaderAction(depth)
	, m_context(context)
	, m_pointerAddress(pointerAddress)
	, m_objectId(objectId)
{}

void ResolvePointerAction::Perform()
{
	auto pointerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_pointerAddress));

	const SerializationContext::ObjectData* objectData = m_context->GetObjectById(m_objectId);
	if (nullptr != objectData)
	{
		auto pointerValueAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(objectData->objectPtr));
		Log::LogMessage("ResolvePointerAction performed. Pointer at 0x%llX filled with address 0x%llX", pointerAsInt, pointerValueAsInt);
		rttr::AssignPointerValue(m_pointerAddress, objectData->objectPtr);
	}
	else
	{
		Log::LogMessage("ResolvePointerAction performed. Pointer at 0x%llX filled with address null", pointerAsInt);
		rttr::AssignPointerValue(m_pointerAddress, nullptr);
	}
}

const ReaderActionType ResolvePointerAction::GetActionType() const
{
	return ReaderActionType::ResolvePtr;
}

} // namespace detail
} // namespace rs
