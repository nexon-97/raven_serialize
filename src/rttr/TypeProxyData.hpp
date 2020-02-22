#pragma once
#include "rttr/ProxyConverter.hpp"
#include "rttr/Type.hpp"
#include <memory>

namespace rttr
{

struct TypeProxyData
{
	Type proxyType;
	std::unique_ptr<ProxyConverterBase> readConverter;
	std::unique_ptr<ProxyConverterBase> writeConverter;

	TypeProxyData(const Type& proxyType)
		: proxyType(proxyType)
	{}

	TypeProxyData(const Type& proxyType, std::unique_ptr<ProxyConverterBase>&& readConverter, std::unique_ptr<ProxyConverterBase>&& writeConverter)
		: proxyType(proxyType)
		, readConverter(std::move(readConverter))
		, writeConverter(std::move(writeConverter))
	{}
};

}
