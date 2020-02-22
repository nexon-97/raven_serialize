#include "rttr/Manager.hpp"
#include "SerializationContext.hpp"

namespace rttr
{

void CollectionResizeNoop(void*, const std::size_t) {}

void InitRavenSerialization()
{
	MetaType<rs::detail::SerializationContext>("@context@");
}

Manager& Manager::GetRTTRManager()
{
	static Manager s_manager;	
	return s_manager;
}

Type Manager::GetMetaTypeByName(const char* name)
{
	auto it = m_typeNames.find(name);
	if (it != m_typeNames.end())
	{
		return Type(it->second);
	}

	return Type(nullptr);
}

Type Manager::GetMetaTypeByTypeIndex(const std::type_index& typeIndex)
{
	auto it = m_types.find(typeIndex);
	if (it != m_types.end())
	{
		return Type(it->second.get());
	}

	return Type(nullptr);
}

void Manager::RegisterProxyType(const Type& type, const Type& proxyType, std::unique_ptr<ProxyConstructorBase>&& proxyConstructor)
{
	m_proxyTypes.emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(proxyType, std::move(proxyConstructor)));
}

TypeProxyData* Manager::GetProxyType(const Type& type)
{
	auto it = m_proxyTypes.find(type);
	if (it != m_proxyTypes.end())
	{
		return &(it->second);
	}

	return nullptr;
}

void Manager::AddTypeDataInternal(const std::type_index& typeIndex, std::unique_ptr<type_data>&& typeData)
{
	type_data* typeDataRaw = typeData.get();
	m_types.emplace(typeIndex, std::move(typeData));

	if (nullptr != typeDataRaw)
	{
		typeDataRaw->hash = m_types.size();
	}
}

Type Reflect(const char* name)
{
	return Manager::GetRTTRManager().GetMetaTypeByName(name);
}

Type Reflect(const std::type_index& typeIndex)
{
	return Manager::GetRTTRManager().GetMetaTypeByTypeIndex(typeIndex);
}

} // namespace rttr
