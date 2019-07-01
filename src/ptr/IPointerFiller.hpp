#pragma once

namespace rs
{
namespace detail
{

class SerializationContext;

class IPointerFiller
{
public:
	explicit IPointerFiller(const SerializationContext* context)
		: m_context(context)
	{}

	virtual void Fill() = 0;

protected:
	const SerializationContext* m_context;
};

} // namespace detail
} // namespace rs
