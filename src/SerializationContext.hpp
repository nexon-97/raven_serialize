#pragma once
#include "rttr/Type.hpp"
#include "rttr/Property.hpp"

namespace rs
{
namespace detail
{

////////////////////////////////////////////////////////////////////////////////////////

class SerializationContext;

class IPointerFiller
{
public:
	explicit IPointerFiller(const SerializationContext* context)
		: m_context(context)
	{}

	virtual void Fill() = 0;

protected:
	const SerializationContext* m_context;
};

class DefaultPointerFiller
	: public IPointerFiller
{
public:
	explicit DefaultPointerFiller(const SerializationContext* context, const rttr::Type& type, const std::size_t objectId, void* pointerAddress);

	void Fill() final;

private:
	rttr::Type m_type;
	std::size_t m_objectId;
	void* m_pointerAddress;
};

class PropertyPointerFiller
	: public IPointerFiller
{
public:
	explicit PropertyPointerFiller(const SerializationContext* context, const std::size_t objectId, void* object, const rttr::Property* property);

	void Fill() final;

private:
	const rttr::Property* m_property;
	void* m_object;
	std::size_t m_objectId;
};

////////////////////////////////////////////////////////////////////////////////////////

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

private:
	std::vector<ObjectData> m_objects;
	std::vector<std::unique_ptr<IPointerFiller>> m_pointerFillers;
};

} // namespace detail
} // namespace rs
