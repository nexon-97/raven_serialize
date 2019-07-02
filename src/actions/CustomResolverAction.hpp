#pragma once
#include "actions/IReaderAction.hpp"
#include "rttr/CustomTypeResolver.hpp"

namespace rs
{
namespace detail
{

class CustomResolverAction
	: public IReaderAction
{
public:
	explicit CustomResolverAction(const std::size_t depth, const rttr::Type& targetType, void* targetValue
		, const rttr::Type& dataType, const void* dataValue, rttr::CustomTypeResolver* resolver);

	void Perform() final;
	const ReaderActionType GetActionType() const final;

private:
	rttr::Type m_targetType;
	rttr::Type m_dataType;
	const void* m_dataValue;
	void* m_targetValue;
	rttr::CustomTypeResolver* m_resolver;
};

} // namespace detail
} // namespace rs
