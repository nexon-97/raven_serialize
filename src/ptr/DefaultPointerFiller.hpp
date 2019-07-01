#pragma once
#include "ptr/IPointerFiller.hpp"
#include "rttr/Type.hpp"

namespace rs
{
namespace detail
{

class DefaultPointerFiller
	: public IPointerFiller
{
public:
	explicit DefaultPointerFiller(const SerializationContext* context, const rttr::Type& type, const std::size_t objectId, void* pointerAddress);

	void Fill() final;

private:
	rttr::Type m_type;
	std::size_t m_objectId;
	void* m_pointerAddress;
};

} // namespace detail
} // namespace rs
