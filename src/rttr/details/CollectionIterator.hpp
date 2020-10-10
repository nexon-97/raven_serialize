#pragma once

namespace rttr
{

struct CollectionIteratorBase
{
	virtual ~CollectionIteratorBase() = default;

	virtual void* operator*() = 0;
	virtual CollectionIteratorBase& operator++() = 0;
	virtual operator bool() const = 0;
};

template <class IteratorT>
struct CollectionIteratorImpl : public CollectionIteratorBase
{
	CollectionIteratorImpl(const IteratorT& inIt, const IteratorT& inEndIt)
		: it(inIt)
		, endIt(inEndIt)
	{}

	void* operator*() override
	{
		return &(*it);
	}

	CollectionIteratorImpl& operator++() override
	{
		it++;
		return *this;
	}

	operator bool() const override
	{
		return it != endIt;
	}

	IteratorT it;
	IteratorT endIt;
};

class CollectionIteratorFactory
{
public:
	virtual std::unique_ptr<CollectionIteratorBase> CreateIterator(void* collection) = 0;
};

template <class CollectionT>
class CollectionIteratorFactoryImpl
	: public CollectionIteratorFactory
{
public:
	std::unique_ptr<CollectionIteratorBase> CreateIterator(void* collection) override
	{
		CollectionT* typedCollection = static_cast<CollectionT*>(collection);
		return std::make_unique<CollectionIteratorImpl<typename CollectionT::iterator>>(typedCollection->begin(), typedCollection->end());
	}
};

}
