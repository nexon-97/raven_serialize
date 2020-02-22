#include "ptr/PropertyPointerFiller.hpp"
#include "SerializationContext.hpp"

namespace rs
{
namespace detail
{

PropertyPointerFiller::PropertyPointerFiller(const SerializationContext* context, const ContextPath& path
	, const std::size_t objectId, void* contextRootPtr, const rttr::Property* property)
	: IPointerFiller(context)
	, m_objectId(objectId)
	, m_contextRootPtr(contextRootPtr)
	, m_property(property)
	, m_path(path)
{}

void PropertyPointerFiller::Fill()
{
	const rttr::Type& propertyType = m_property->GetType();
	void* tempPointer = propertyType.Instantiate();

	const SerializationContext::ObjectData* objectData = m_context->GetObjectById(m_objectId);
	if (nullptr != objectData)
	{
		//propertyType.AssignPointerValue(tempPointer, const_cast<void*>(objectData->objectPtr));
	}
	else
	{
		//propertyType.AssignPointerValue(tempPointer, nullptr);
	}

	//auto propertyData = m_path.ResolvePropertyData();

	//m_property->CallMutator(m_object, tempPointer);

	propertyType.Destroy(tempPointer);
}

} // namespace detail
} // namespace rs
