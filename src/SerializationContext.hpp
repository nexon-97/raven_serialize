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

	struct PointerToFillData
	{
		rttr::Type type;
		std::size_t objectId;
		void* pointerAddress;

		RAVEN_SER_API PointerToFillData(const rttr::Type& type, const std::size_t objectId, void* pointerAddress) noexcept;
	};

	std::size_t RAVEN_SER_API AddObject(const rttr::Type& type, const void* objectPtr);
	void RAVEN_SER_API AddPointerToFill(const rttr::Type& type, const std::size_t objectId, void* pointerAddress);
	RAVEN_SER_API ObjectData* GetObjectById(const std::size_t id);
	RAVEN_SER_API const std::vector<ObjectData>& GetObjects() const;
	RAVEN_SER_API const std::vector<PointerToFillData>& GetPointersToFill() const;

private:
	std::vector<ObjectData> m_objects;
	std::vector<PointerToFillData> m_pointersToFill;
};

} // namespace detail
} // namespace rs
