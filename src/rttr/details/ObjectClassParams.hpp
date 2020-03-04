#pragma once
#include "rttr/Property.hpp"
#include "rttr/details/CollectionInserter.hpp"
#include <vector>
#include <unordered_map>
#include <array>
#include <iterator>

namespace rttr
{

///////////////////////////////////////////////////////////////////////////////////

struct CollectionParams
{
	std::unique_ptr<CollectionInserterFactory> inserterFactory;
	Type itemType;
};

struct ObjectClassParams
{
	std::vector<std::unique_ptr<Property>> properties;
	std::unique_ptr<CollectionParams> collectionParams;
	bool isPolymorphic = false;
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

template <typename T, typename U>
struct CollectionTraitsResolver<std::unordered_map<T, U>>
{
	void operator()(ObjectClassParams& params)
	{
		params.collectionParams = std::make_unique<CollectionParams>();

		using InserterT = typename CollectionStdInserter<std::unordered_map<T, U>, std::pair<T, U>>;
		auto inserterFactory = std::make_unique<CollectionInserterFactoryImpl<InserterT>>();
		params.collectionParams->inserterFactory = std::move(inserterFactory);

		params.collectionParams->itemType = Reflect<std::pair<T, U>>();
	}
};

template <typename T, std::size_t Size>
struct CollectionTraitsResolver<std::array<T, Size>>
{
	void operator()(ObjectClassParams& params)
	{
		params.collectionParams = std::make_unique<CollectionParams>();

		using InserterT = typename StdArrayInserter<T, Size>;
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
		params.isPolymorphic = std::is_polymorphic_v<T>;

		CollectionTraitsResolver<T> collectionTraitsResolver;
		collectionTraitsResolver(params);
	}
};

}
