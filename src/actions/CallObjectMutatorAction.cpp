#include "actions/CallObjectMutatorAction.hpp"
#include "rs/log/Log.hpp"

namespace rs
{
namespace detail
{

CallObjectMutatorAction::CallObjectMutatorAction(const std::size_t depth, rttr::Property* property, void* object, const void* assignedValue)
	: IReaderAction(depth)
	, m_property(property)
	, m_object(object)
	, m_assignedValue(assignedValue)
{}

void CallObjectMutatorAction::Perform()
{
	auto objectPtrAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_object));
	auto valuePtrAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_assignedValue));
	Log::LogMessage("CallObjectMutatorAction performed. Object 0x%llX property '%s', value at 0x%llX", objectPtrAsInt, m_property->GetName(), valuePtrAsInt);

	m_property->CallMutator(m_object, const_cast<void*>(m_assignedValue));
}

const ReaderActionType CallObjectMutatorAction::GetActionType() const
{
	return ReaderActionType::CallMutator;
}

} // namespace detail
} // namespace rs
