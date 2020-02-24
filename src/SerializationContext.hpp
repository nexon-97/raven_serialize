#pragma once
#include "rttr/Type.hpp"
#include "rttr/Property.hpp"

#include <unordered_map>

namespace rs
{
namespace detail
{

class SerializationContext
{
public:
	SerializationContext() = default; 
	~SerializationContext();

	struct ObjectData
	{
		rttr::Type type;
		void* objectPtr;

		RAVEN_SERIALIZE_API ObjectData(const rttr::Type& type, void* objectPtr) noexcept;
	};

	void RAVEN_SERIALIZE_API AddObject(const uint64_t idx, const rttr::Type& type, void* objectPtr);
	RAVEN_SERIALIZE_API ObjectData const* GetObjectById(const uint64_t id) const;

	RAVEN_SERIALIZE_API void* CreateTempVariable(const rttr::Type& type);
	void RAVEN_SERIALIZE_API DestroyTempVariable(void* ptr);
	void RAVEN_SERIALIZE_API ClearTempVariables();

private:
	std::unordered_map<uint64_t, ObjectData> m_objects;
	std::unordered_map<void*, rttr::Type> m_tempVariables;
};

} // namespace detail
} // namespace rs
