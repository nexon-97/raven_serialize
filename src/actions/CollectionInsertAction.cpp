#include "actions/CollectionInsertAction.hpp"
#include "rs/log/Log.hpp"

namespace rs
{
namespace detail
{

CollectionInsertAction::CollectionInsertAction(const std::size_t depth, std::unique_ptr<rttr::CollectionInserterBase>&& inserter, const void* value)
	: IReaderAction(depth)
	, m_value(value)
	, m_inserter(std::move(inserter))
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
