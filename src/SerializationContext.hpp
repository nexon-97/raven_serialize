#pragma once
#include "rttr/Type.hpp"

namespace rs
{
namespace detail
{

class SerializationContext
{
public:
	SerializationContext() = default;

	struct ObjectData
	{
		rttr::Type type;
		const void* objectPtr;
		std::size_t objectId;

		RAVEN_SER_API ObjectData(const rttr::Type& type, const void* objectPtr, const std::size_t objectId) noexcept;
	};

	std::size_t RAVEN_SER_API AddObject(const rttr::Type& type, const void* objectPtr);
	RAVEN_SER_API ObjectData* GetObjectById(const std::size_t id);
	RAVEN_SER_API const std::vector<ObjectData>& GetObjects() const;

private:
	std::vector<ObjectData> m_objects;
};

} // namespace detail
} // namespace rs
