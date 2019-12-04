#pragma once
#include "raven_serialize.hpp"
#include <tuple>

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

protected:
	Type* m_argTypes = nullptr;
	int m_argsCount = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////

/*template <typename T, typename ...Args>
std::unique_ptr<T> CreateUniqueInstanceWithArgs(Args&&... args)
{
	static UniqueInstanceConstructor<T, Args> instanceConstructor;
	return instanceConstructor.Construct(std::forward<Args>(args)...);*/

template <class T, typename ...Args>
T* NewWrapper(Args&&... args)
{
	return new T(std::forward(args)...);
}

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
		/*std::size_t currentArgOffset = 0;

		for (int paramIdx = 0; paramIdx < m_argsCount; ++paramIdx)
		{
			std::size_t argSize = m_argTypes[paramIdx].GetSize();
			
			
			currentArgOffset += argSize;
		}

		std::tuple<Args...> parameters;*/
		T* instance = std::apply(NewWrapper<T, Args...>, *parametersTuple);

		return instance;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////

} // namespace rttr
