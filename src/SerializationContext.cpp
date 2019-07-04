#include "SerializationContext.hpp"
#include "rs/log/Log.hpp"

namespace rs
{
namespace detail
{

SerializationContext::ObjectData::ObjectData(const rttr::Type& type, const void* objectPtr, const std::size_t objectId) noexcept
	: type(type)
	, objectPtr(objectPtr)
	, objectId(objectId)
{}

std::size_t SerializationContext::AddObject(const rttr::Type& type, const void* objectPtr)
{
	auto predicate = [&type, objectPtr](const ObjectData& data)
	{
		return data.objectPtr == objectPtr && data.type == type;
	};

	// Find the same object in context
	auto it = std::find_if(m_objects.begin(), m_objects.end(), predicate);
	if (it == m_objects.end())
	{
		// Ojbect not found, create new entry
		std::size_t objectId = m_objects.size();
		m_objects.emplace_back(type, objectPtr, objectId);

		return objectId;
	}
	else
	{
		// Object found, return its id
		return it->objectId;
	}
}

void SerializationContext::AddObject(const std::size_t idx, const rttr::Type& type, const void* objectPtr)
{
	m_objects.emplace_back(type, objectPtr, idx);
}

const SerializationContext::ObjectData* SerializationContext::GetObjectById(const std::size_t id) const
{
	auto predicate = [id](const ObjectData& data)
	{
		return data.objectId == id;
	};

	auto it = std::find_if(m_objects.begin(), m_objects.end(), predicate);
	if (it != m_objects.end())
	{
		const SerializationContext::ObjectData& data = *it;
		return &data;
	}

	return nullptr;
}

const std::vector<SerializationContext::ObjectData>& SerializationContext::GetObjects() const
{
	return m_objects;
}

void* SerializationContext::CreateTempVariable(const rttr::Type& type)
{
	void* temp = type.Instantiate();

	auto varAddressAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(temp));
	Log::LogMessage("Temp variable created: (%s; 0x%llX)", type.GetName(), varAddressAsInt);

	m_tempVariables.emplace_back(type, temp);
	return temp;
}

void SerializationContext::ClearTempVariables()
{
	for (const auto& tempVar : m_tempVariables)
	{
		auto varAddressAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(tempVar.second));
		Log::LogMessage("Temp variable destroyed: (%s; 0x%llX)", tempVar.first.GetName(), varAddressAsInt);

		tempVar.first.Destroy(tempVar.second);
	}
	m_tempVariables.clear();
}

} // namespace detail
} // namespace rs
