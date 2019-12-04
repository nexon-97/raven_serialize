#include "Constructor.hpp"
#include "Type.hpp"

namespace rttr
{

Constructor::Constructor(Type* argTypes, int argsCount)
{
	if (argsCount > 0 && argTypes != nullptr)
	{
		m_argTypes = new Type[argsCount];
		m_argsCount = argsCount;

		for (int i = 0; i < m_argsCount; ++i)
		{
			m_argTypes[i] = argTypes[i];
		}
	}
}

Constructor::~Constructor()
{
	if (nullptr != m_argTypes)
	{
		delete[] m_argTypes;
		m_argTypes = nullptr;
		m_argsCount = 0;
	}
}

}
