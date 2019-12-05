#pragma once
#include "raven_serialize.hpp"
#include <tuple>
#include <memory>

namespace rttr
{

class Type;

///////////////////////////////////////////////////////////////////////////////////////

class Constructor
{
public:
	RAVEN_SER_API Constructor(Type* argTypes, int argsCount);
	RAVEN_SER_API ~Constructor();

	virtual void* Construct(void* paramsData) = 0;

	template <class BaseClass>
	std::unique_ptr<BaseClass> ConstructUnique(void* paramsData)
	{
		BaseClass* instance = static_cast<BaseClass*>(Construct(paramsData));
		return std::unique_ptr<BaseClass>(instance);
	}

	virtual std::shared_ptr<void> ConstructShared(void* paramsData) = 0;

	bool RAVEN_SER_API operator==(const Constructor& other) const;

protected:
	friend class Type;

	Type* m_argTypes = nullptr;
	int m_argsCount = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////

namespace detail
{

template <class T, class Tuple, std::size_t... I>
constexpr T* make_from_tuple_impl(Tuple&& t, std::index_sequence<I...>)
{
	return new T(std::get<I>(std::forward<Tuple>(t))...);
}

template <class T, class Tuple>
constexpr T* make_from_tuple(Tuple&& t)
{
	return detail::make_from_tuple_impl<T>(std::forward<Tuple>(t),
		std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

template <class T, class Tuple, std::size_t... I>
constexpr std::shared_ptr<T> make_shared_from_tuple_impl(Tuple&& t, std::index_sequence<I...>)
{
	return std::make_shared<T>(std::get<I>(std::forward<Tuple>(t))...);
}

template <class T, class Tuple>
constexpr std::shared_ptr<T> make_shared_from_tuple(Tuple&& t)
{
	return detail::make_shared_from_tuple_impl<T>(std::forward<Tuple>(t),
		std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

} // namespace detail

template <class T, typename ...Args>
class ConcreteConstructor
	: public Constructor
{
public:
	ConcreteConstructor(Type* argTypes, int argsCount)
		: Constructor(argTypes, argsCount)
	{}

	void* Construct(void* paramsData) override
	{
		std::tuple<Args...>* parametersTuple = reinterpret_cast<std::tuple<Args...>*>(paramsData);
		T* instance = detail::make_from_tuple<T>(*parametersTuple);
		return instance;
	}

	std::shared_ptr<void> ConstructShared(void* paramsData) override
	{
		std::tuple<Args...>* parametersTuple = reinterpret_cast<std::tuple<Args...>*>(paramsData);
		std::shared_ptr<T> instance = detail::make_shared_from_tuple<T>(*parametersTuple);
		return instance;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////

} // namespace rttr
