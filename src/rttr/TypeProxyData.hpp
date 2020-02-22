#pragma once
#include "rttr/ProxyConstructor.hpp"
#include "rttr/Type.hpp"
#include <memory>

namespace rttr
{

struct TypeProxyData
{
	Type proxyType;
	std::unique_ptr<ProxyConstructorBase> conversionConstructor;

	TypeProxyData(const Type& proxyType, std::unique_ptr<ProxyConstructorBase>&& conversionConstructor)
		: proxyType(proxyType)
		, conversionConstructor(std::move(conversionConstructor))
	{}
};

}
