#pragma once
#include "actions/ReaderActionType.hpp"
#include <cstddef>

namespace rs
{
namespace detail
{

class IReaderAction
{
public:
	virtual ~IReaderAction() = default;

	explicit IReaderAction(const std::size_t depth)
		: m_depth(depth)
	{}

	virtual void Perform() = 0;
	virtual const ReaderActionType GetActionType() const = 0;

	const std::size_t GetDepth() const
	{
		return m_depth;
	}

protected:
	const std::size_t m_depth;
};

} // namespace detail
} // namespace rs
