#pragma once
#include "actions/IReaderAction.hpp"
#include "rttr/Property.hpp"
#include "rttr/details/CollectionInserter.hpp"

namespace rs
{
namespace detail
{

class CollectionInsertAction
	: public IReaderAction
{
public:
	explicit CollectionInsertAction(const std::size_t depth, rttr::CollectionInserterBase* inserter, const void* value);

	void Perform() final;
	const ReaderActionType GetActionType() const final;

private:
	rttr::CollectionInserterBase* m_inserter;
	const void* m_value;
};

} // namespace detail
} // namespace rs
