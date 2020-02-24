#include "SerializationContext.hpp"
#include "rs/log/Log.hpp"

namespace rs
{
namespace detail
{

SerializationContext::ObjectData::ObjectData(const rttr::Type& type, void* objectPtr) noexcept
	: type(type)
	, objectPtr(objectPtr)
{}

SerializationContext::~SerializationContext()
{
	ClearTempVariables();
}

void SerializationContext::AddObject(const uint64_t idx, const rttr::Type& type, void* objectPtr)
{
	m_objects.emplace(std::piecewise_construct, std::forward_as_tuple(idx), std::forward_as_tuple(type, objectPtr));
}

SerializationContext::ObjectData const* SerializationContext::GetObjectById(const uint64_t id) const
{
	auto it = m_objects.find(id);
	if (it != m_objects.end())
	{
		return &it->second;
	}

	return nullptr;
}

void* SerializationContext::CreateTempVariable(const rttr::Type& type)
{
	void* temp = type.Instantiate();

	auto varAddressAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(temp));
	Log::LogMessage("Temp variable created: (%s; 0x%llX)", type.GetName(), varAddressAsInt);

	m_tempVariables.emplace(temp, type);
	return temp;
}

void SerializationContext::DestroyTempVariable(void* ptr)
{
	auto it = m_tempVariables.find(ptr);
	if (it != m_tempVariables.end())
	{
		auto varAddressAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(ptr));
		Log::LogMessage("Temp variable destroyed: (%s; 0x%llX)", it->second.GetName(), varAddressAsInt);

		it->second.Destroy(ptr);
		m_tempVariables.erase(it);
	}
}

void SerializationContext::ClearTempVariables()
{
	for (const auto& tempVar : m_tempVariables)
	{
		auto varAddressAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(tempVar.first));
		Log::LogMessage("Temp variable destroyed: (%s; 0x%llX)", tempVar.second.GetName(), varAddressAsInt);

		tempVar.second.Destroy(tempVar.first);
	}
	m_tempVariables.clear();
}

} // namespace detail
} // namespace rs
