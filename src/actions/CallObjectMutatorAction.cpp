#include "actions/CallObjectMutatorAction.hpp"

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
	m_property->CallMutator(m_object, const_cast<void*>(m_assignedValue));
}

const ReaderActionType CallObjectMutatorAction::GetActionType() const
{
	return ReaderActionType::CallMutator;
}

} // namespace detail
} // namespace rs
