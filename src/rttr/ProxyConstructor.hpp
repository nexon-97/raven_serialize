#pragma once

namespace rttr
{

class ProxyConstructorBase
{
public:
	virtual void CreateFromProxy(void* target, void* proxyObject) = 0;
};

template <typename T, typename U>
class ProxyConstructorImpl
	: public ProxyConstructorBase
{
	using RawU = typename std::remove_reference_t<U>;

public:
	void CreateFromProxy(void* target, void* proxyObject) override
	{
		RawU* typedProxy = static_cast<RawU*>(proxyObject);
		T* typedTarget = static_cast<T*>(target);
		*typedTarget = T(std::forward<RawU>(*typedProxy));
	}
};

}
