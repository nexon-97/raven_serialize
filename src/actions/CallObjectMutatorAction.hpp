#pragma once
#include "actions/IReaderAction.hpp"
#include "rttr/Property.hpp"

namespace rs
{
namespace detail
{

class CallObjectMutatorAction
	: public IReaderAction
{
public:
	explicit CallObjectMutatorAction(const std::size_t depth, rttr::Property* property, void* object, const void* assignedValue);

	void Perform() final;
	const ReaderActionType GetActionType() const final;

private:
	void* m_object;
	const void* m_assignedValue;
	rttr::Property* m_property;
};

} // namespace detail
} // namespace rs
