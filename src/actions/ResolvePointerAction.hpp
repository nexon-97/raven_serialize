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
	explicit ResolvePointerAction(const std::size_t depth, SerializationContext* context, void* pointerAddress, const uint64_t objectId);

	void Perform() final;
	const ReaderActionType GetActionType() const final;

private:
	SerializationContext* m_context;
	void* m_pointerAddress;
	uint64_t m_objectId;
};

} // namespace detail
} // namespace rs
