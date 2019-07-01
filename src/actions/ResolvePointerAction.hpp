#pragma once
#include "actions/IReaderAction.hpp"
#include "rttr/Property.hpp"

namespace rs
{
namespace detail
{

class SerializationContext;

class ResolvePointerAction
	: public IReaderAction
{
public:
	explicit ResolvePointerAction(const std::size_t depth, SerializationContext* context
		, const rttr::Type& pointerType, void* pointerAddress, const std::size_t markerId);

	void Perform() final;
	const ReaderActionType GetActionType() const final;

private:
	SerializationContext* m_context;
	rttr::Type m_pointerType;
	void* m_pointerAddress;
	std::size_t m_markerId;
};

} // namespace detail
} // namespace rs
