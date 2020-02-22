#include "actions/CollectionInsertAction.hpp"
#include "rs/log/Log.hpp"

namespace rs
{
namespace detail
{

CollectionInsertAction::CollectionInsertAction(const std::size_t depth, rttr::CollectionInserterBase* inserter, const void* value)
	: IReaderAction(depth)
	, m_value(value)
	, m_inserter(inserter)
{}

void CollectionInsertAction::Perform()
{
	m_inserter->Insert(m_value);
}

const ReaderActionType CollectionInsertAction::GetActionType() const
{
	return ReaderActionType::InsertCollectionItem;
}

} // namespace detail
} // namespace rs
