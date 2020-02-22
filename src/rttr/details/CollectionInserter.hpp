#pragma once
#include <iterator>

namespace rttr
{

struct CollectionInserterBase
{
	virtual void Insert(const void* itemObject) = 0;
};

template <class CollectionT, class ItemT>
struct CollectionStdBackInserter
	: public CollectionInserterBase
{
	std::back_insert_iterator<CollectionT> m_insertIterator;

	CollectionStdBackInserter(void* collection)
		: m_insertIterator(std::back_inserter(*static_cast<CollectionT*>(collection)))
	{}

	void Insert(const void* itemObject) override
	{
		const ItemT* item = static_cast<const ItemT*>(itemObject);
		m_insertIterator = *item;
	}
};

///////////////////////////////////////////////////////////////////////////////////

class CollectionInserterFactory
{
public:
	virtual std::unique_ptr<CollectionInserterBase> CreateInserter(void* collection) = 0;
};

template <class InserterT>
class CollectionInserterFactoryImpl
	: public CollectionInserterFactory
{
public:
	std::unique_ptr<CollectionInserterBase> CreateInserter(void* collection) override
	{
		return std::make_unique<InserterT>(collection);
	}
};

///////////////////////////////////////////////////////////////////////////////////

}
