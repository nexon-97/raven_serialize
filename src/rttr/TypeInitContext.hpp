#pragma once
#include "rttr/Type.hpp"
#include "rttr/ProxyConverter.hpp"

namespace rttr
{

template <typename T>
class TypeInitContext
{
public:
	explicit TypeInitContext(const Type& type)
		: m_generatedType(type)
	{}

	// Mark this type is serialized/deserialized as if it was another declared type
	// To deserialize the type it have to declare a constructor that accepts proxy type lvalue or rvalue
	// To serialize this type to proxy type, conversion function must be provided
	template <typename U>
	TypeInitContext& DeclProxyBehavior()
	{
		static_assert(!std::is_same_v<T, U>, "Proxy type can't be the same as type!");
		static_assert(std::is_constructible_v<T, U>, "Type must have constructor taking proxy object type to work!");

		// Check if current type doesn't have proxy, as multiple proxies are not allowed
		assert(nullptr == m_generatedType.GetProxyType());

		Type proxyType = Reflect<U>();
		m_generatedType.RegisterProxy(proxyType);

		return *this;
	}

	template <typename U>
	TypeInitContext& SetDefaultProxyReadConverter()
	{
		TypeProxyData* proxyTypeData = m_generatedType.GetProxyType();
		assert(nullptr != proxyTypeData);

		proxyTypeData->readConverter = std::make_unique<ConstructorProxyConverter<T, U>>();
		return *this;
	}

	template <typename ConverterT, typename ...Args>
	TypeInitContext& SetProxyReadConverter(Args&&... args)
	{
		static_assert(std::is_base_of_v<ProxyConverterBase, ConverterT>, "Proxy converter must be derived from ProxyConverterBase!");

		TypeProxyData* proxyTypeData = m_generatedType.GetProxyType();
		assert(nullptr != proxyTypeData);

		proxyTypeData->readConverter = std::make_unique<ConverterT>(std::forward<Args>(args)...);
		return *this;
	}

	template <typename U>
	TypeInitContext& SetDefaultProxyWriteConverter()
	{
		TypeProxyData* proxyTypeData = m_generatedType.GetProxyType();
		assert(nullptr != proxyTypeData);

		proxyTypeData->writeConverter = std::make_unique<ConstructorProxyConverter<U, T>>();
		return *this;
	}

	template <typename ConverterT, typename MemberFuncPrototype>
	TypeInitContext& SetProxyWriteConverter(MemberFuncPrototype memFnPrototype)
	{
		static_assert(std::is_base_of_v<ProxyConverterBase, ConverterT>, "Proxy converter must be derived from ProxyConverterBase!");

		TypeProxyData* proxyTypeData = m_generatedType.GetProxyType();
		assert(nullptr != proxyTypeData);

		proxyTypeData->writeConverter = std::make_unique<ConverterT>(memFnPrototype);
		return *this;
	}

	template <typename Signature>
	TypeInitContext& DeclProperty(const char* name, Signature signature)
	{
		std::shared_ptr<Property> property = CreateMemberProperty(name, signature);
		m_generatedType.AddProperty(std::move(property));

		return *this;
	}

	template <typename GetterSignature, typename SetterSignature>
	TypeInitContext& DeclProperty(const char* name, GetterSignature getter, SetterSignature setter)
	{
		std::shared_ptr<Property> property = CreateIndirectProperty(name, getter, setter);
		m_generatedType.AddProperty(std::move(property));

		return *this;
	}

private:
	Type m_generatedType;
};

}
