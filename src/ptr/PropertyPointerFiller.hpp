#pragma once
#include "ptr/IPointerFiller.hpp"
#include "rttr/Property.hpp"
#include "ContextPath.hpp"

namespace rs
{
namespace detail
{

class PropertyPointerFiller
	: public IPointerFiller
{
public:
	explicit PropertyPointerFiller(const SerializationContext* context, const ContextPath& path
		, const std::size_t objectId, void* contextRootPtr, const rttr::Property* property);

	void Fill() final;

private:
	ContextPath m_path;
	const rttr::Property* m_property;
	void* m_contextRootPtr;
	std::size_t m_objectId;
};

} // namespace detail
} // namespace rs
