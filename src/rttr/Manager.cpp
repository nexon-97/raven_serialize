#include "rttr/Manager.hpp"
#include "SerializationContext.hpp"

namespace
{
rttr::Manager* g_managerInstance = nullptr;
}

namespace rttr
{

void CollectionResizeNoop(void*, const std::size_t) {}

void InitRavenSerialization()
{
	Manager::InitRTTR();
	MetaType<rs::detail::SerializationContext>("@context@");
}

void AssignPointerValue(void* pointerAddress, void* value)
{
	uintptr_t** pointerValue = reinterpret_cast<uintptr_t**>(pointerAddress);
	*pointerValue = static_cast<uintptr_t*>(value);
}

void Manager::InitRTTR()
{
	g_managerInstance = new Manager();
}

void Manager::DestroyRTTR()
{
	if (nullptr != g_managerInstance)
	{
		delete g_managerInstance;
		g_managerInstance = nullptr;
	}
}

Manager& Manager::GetRTTRManager()
{
	return *g_managerInstance;
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

void Manager::RegisterProxyType(const Type& type, const Type& proxyType)
{
	m_proxyTypes.emplace(type, std::make_unique<TypeProxyData>(proxyType));
	m_customSerializationTypes[type.GetId()] = rs::SerializationMethod::Proxy;
}

TypeProxyData* Manager::GetProxyType(const Type& type)
{
	auto it = m_proxyTypes.find(type);
	if (it != m_proxyTypes.end())
	{
		return it->second.get();
	}

	return nullptr;
}

void Manager::RegisterSerializationAdapter(const Type& type, std::unique_ptr<rs::SerializationAdapter>&& adapter)
{
	m_serializationAdapters.emplace(type, std::move(adapter));
	m_customSerializationTypes[type.GetId()] = rs::SerializationMethod::Adapter;
}

rs::SerializationAdapter* Manager::GetSerializationAdapter(const Type& type)
{
	auto it = m_serializationAdapters.find(type);
	if (it != m_serializationAdapters.end())
	{
		return it->second.get();
	}

	return nullptr;
}

rs::SerializationMethod Manager::GetSerializationMethod(const Type& type) const
{
	auto it = m_customSerializationTypes.find(type.GetId());
	if (it != m_customSerializationTypes.end())
	{
		return it->second;
	}

	return rs::SerializationMethod::Default;
}

void Manager::AddTypeDataInternal(const std::type_index& typeIndex, std::unique_ptr<type_data>&& typeData)
{
	m_types.emplace(typeIndex, std::move(typeData));
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
