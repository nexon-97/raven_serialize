#include "actions/CustomResolverAction.hpp"
#include "rs/log/Log.hpp"

namespace rs
{
namespace detail
{

CustomResolverAction::CustomResolverAction(const std::size_t depth, const rttr::Type& targetType, void* targetValue
	, const rttr::Type& dataType, const void* dataValue, rttr::CustomTypeResolver* resolver)
	: IReaderAction(depth)
	, m_targetType(targetType)
	, m_targetValue(targetValue)
	, m_dataType(dataType)
	, m_dataValue(dataValue)
	, m_resolver(resolver)
{}

void CustomResolverAction::Perform()
{
	auto targetPtrAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_targetValue));
	auto dataPtrAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_dataValue));
	Log::LogMessage("CustomResolverAction performed. Data value (%s; 0x%llX) resolved to (%s; 0x%llX)"
		, m_dataType.GetName(), dataPtrAsInt, m_targetType.GetName(), targetPtrAsInt);

	std::uintptr_t* pointerAddress = reinterpret_cast<std::uintptr_t*>(m_targetValue);
	auto result = m_resolver->ResolveReverse(m_targetType, m_dataType, pointerAddress, m_dataValue);
}

const ReaderActionType CustomResolverAction::GetActionType() const
{
	return ReaderActionType::CallMutator;
}

} // namespace detail
} // namespace rs
