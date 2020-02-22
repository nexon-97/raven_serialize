#pragma once
#include "rttr/Property.hpp"
#include "rttr/details/CollectionInserter.hpp"
#include <vector>
#include <iterator>

namespace rttr
{

///////////////////////////////////////////////////////////////////////////////////

//template <class It, class T>
//using CollectionIteratorGetter = It(T::*)();
//
//template <class T, class It>
//struct CollectionIterationData
//{
//	CollectionIteratorGetter<It, T> beginMethod;
//	CollectionIteratorGetter<It, T> endMethod;
//};
//
//struct CollectionWriteContext
//{
//	//virtual std::vector<> WriteCollection(void* collection);
//};

struct CollectionParams
{
	std::unique_ptr<CollectionInserterFactory> inserterFactory;
	Type itemType;
};

struct ObjectClassParams
{
	std::vector<std::unique_ptr<Property>> properties;
	std::unique_ptr<CollectionParams> collectionParams;
};

///////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Cond = void>
struct CollectionTraitsResolver
{
	void operator()(ObjectClassParams& params) {}
};

template <typename T>
struct CollectionTraitsResolver<std::vector<T>>
{
	void operator()(ObjectClassParams& params)
	{
		params.collectionParams = std::make_unique<CollectionParams>();

		using InserterT = typename CollectionStdBackInserter<std::vector<T>, T>;
		auto inserterFactory = std::make_unique<CollectionInserterFactoryImpl<InserterT>>();
		params.collectionParams->inserterFactory = std::move(inserterFactory);

		params.collectionParams->itemType = Reflect<T>();
	}
};

///////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Cond = void>
struct ObjectTraitsResolver
{
	void operator()(ObjectClassParams& params) {}
};

template <typename T>
struct ObjectTraitsResolver<T, std::enable_if_t<std::is_class_v<T>>>
{
	void operator()(ObjectClassParams& params)
	{
		CollectionTraitsResolver<T> collectionTraitsResolver;
		collectionTraitsResolver(params);
	}
};

}
