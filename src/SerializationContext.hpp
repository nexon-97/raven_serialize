#pragma once
#include "rttr/Type.hpp"
#include "rttr/Property.hpp"
#include "ptr/IPointerFiller.hpp"

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
	void RAVEN_SER_API AddObject(const std::size_t idx, const rttr::Type& type, const void* objectPtr);
	void RAVEN_SER_API AddPointerFiller(std::unique_ptr<IPointerFiller>&& pointerFiller);
	RAVEN_SER_API const ObjectData* GetObjectById(const std::size_t id) const;
	RAVEN_SER_API const std::vector<ObjectData>& GetObjects() const;
	RAVEN_SER_API const std::vector<std::unique_ptr<IPointerFiller>>& GetPointerFillers() const;

	RAVEN_SER_API void* CreateTempVariable(const rttr::Type& type);
	void RAVEN_SER_API ClearTempVariables();

private:
	std::vector<ObjectData> m_objects;
	std::vector<std::unique_ptr<IPointerFiller>> m_pointerFillers;
	std::vector<std::pair<rttr::Type, void*>> m_tempVariables;
};

} // namespace detail
} // namespace rs
