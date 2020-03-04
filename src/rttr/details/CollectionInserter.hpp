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

template <class CollectionT, class ItemT>
struct CollectionStdInserter
	: public CollectionInserterBase
{
	std::insert_iterator<CollectionT> m_insertIterator;

	CollectionStdInserter(void* collection)
		: m_insertIterator(std::inserter(*static_cast<CollectionT*>(collection), static_cast<CollectionT*>(collection)->end()))
	{}

	void Insert(const void* itemObject) override
	{
		const ItemT* item = static_cast<const ItemT*>(itemObject);
		m_insertIterator = *item;
	}
};

template <class ItemT, std::size_t Size>
struct StdArrayInserter
	: public CollectionInserterBase
{
	using ArrayT = std::array<ItemT, Size>;

	ArrayT* m_array;
	std::size_t m_currentIndex;

	StdArrayInserter(void* collection)
		: m_array(reinterpret_cast<ArrayT*>(collection))
		, m_currentIndex(0U)
	{}

	void Insert(const void* itemObject) override
	{
		const ItemT* item = static_cast<const ItemT*>(itemObject);
		m_array->at(m_currentIndex) = *item;
		++m_currentIndex;
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
