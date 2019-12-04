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

protected:
	Type* m_argTypes = nullptr;
	int m_argsCount = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////

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
		//T* instance = std::apply(NewWrapper<T, Args...>, *parametersTuple);

		return new T();
	}

	std::shared_ptr<void> ConstructShared(void* paramsData) override
	{
		std::tuple<Args...>* parametersTuple = reinterpret_cast<std::tuple<Args...>*>(paramsData);
		return std::make_shared<T>();
	}
};

////////////////////////////////////////////////////////////////////////////////////////////

} // namespace rttr
