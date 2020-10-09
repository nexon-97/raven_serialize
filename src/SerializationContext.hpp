#pragma once
#include "rttr/Type.hpp"
#include "rttr/Property.hpp"

#include <unordered_map>

namespace rs
{
namespace detail
{

/*
* @brief Serialization context is an optional state of serialization process
* 
* Structure:
* - Serialization objects represented with void* data pointer and meta type describing how to treat that data
* - Temp variables need to handle indirect properties (for example properties defined with getter/setter pair
* are first resolved into temp variable, and then that temp variable is assigned to the target using setter method,
* and then temp variable can be discarded
*/
class SerializationContext
{
public:
	SerializationContext() = default; 
	RAVEN_SERIALIZE_API ~SerializationContext();

	struct ObjectData
	{
		rttr::Type type;
		void* objectPtr;

		RAVEN_SERIALIZE_API ObjectData(const rttr::Type& type, void* objectPtr) noexcept;
	};

	// Object store manipulation
	void RAVEN_SERIALIZE_API AddObject(const uint64_t idx, const rttr::Type& type, void* objectPtr);
	RAVEN_SERIALIZE_API ObjectData const* GetObjectById(const uint64_t id) const;

	// Temp variables manipulation
	RAVEN_SERIALIZE_API void* CreateTempVariable(const rttr::Type& type);
	void RAVEN_SERIALIZE_API DestroyTempVariable(void* ptr);
	void RAVEN_SERIALIZE_API ClearTempVariables();

private:
	std::unordered_map<uint64_t, ObjectData> m_objects;
	std::unordered_map<void*, rttr::Type> m_tempVariables;
};

} // namespace detail
} // namespace rs
