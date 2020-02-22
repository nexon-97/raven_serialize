#pragma once
#include "rttr/Type.hpp"
#include "rttr/ProxyConverter.hpp"

namespace rttr
{

template <typename Signature>
struct PropertyCreatorFromSignature
{
	std::unique_ptr<Property> operator()(const char* name, Signature memberPointerSignature)
	{
		return CreateMemberProperty(name, memberPointerSignature);
	}
};

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
		static_assert(std::is_same_v<ExtractClassType<Signature>::type, T>, "Member signature class type doesn't match!");

		PropertyCreatorFromSignature<Signature> propertyCreator;
		std::unique_ptr<Property> propertyInstance = propertyCreator(name, signature);
		m_generatedType.AddProperty(std::move(propertyInstance));

		return *this;
	}

	template <typename GetterSignature, typename SetterSignature>
	TypeInitContext& DeclProperty(const char* name, GetterSignature getter, SetterSignature setter)
	{
		static_assert(IsMemberFuncPrototype<GetterSignature>::value, "GetterSignature must be member function prototype");
		static_assert(IsMemberFuncPrototype<SetterSignature>::value, "SetterSignature must be member function prototype");
		static_assert(std::is_same_v<ExtractClassType<GetterSignature>::type, T>, "Member function prototype doesn't match type!");
		static_assert(std::is_same_v<ExtractClassType<SetterSignature>::type, T>, "Proxy converter must be derived from ProxyConverterBase!");

		std::unique_ptr<Property> propertyInstance = CreateIndirectProperty(name, getter, setter);
		m_generatedType.AddProperty(std::move(propertyInstance));

		return *this;
	}

	Type& DeclProperty(const char* name, rs::ICustomPropertyResolvePolicy* policy)
	{
		std::unique_ptr<Property> propertyInstance = std::make_unique<rttr::CustomProperty>(name, policy, policy->GetType());
		m_generatedType.AddProperty(std::move(propertyInstance));

		return *this;
	}

private:
	Type m_generatedType;
};

}
