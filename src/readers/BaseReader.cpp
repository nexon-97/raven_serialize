#include "readers/BaseReader.hpp"
#include "rs/log/Log.hpp"

namespace rs
{

void BaseReader::Read(const rttr::Type& type, void* value)
{
	m_context = std::make_unique<rs::detail::SerializationContext>();

	//bool hasObjectsList = CheckSourceHasObjectsList();
	//if (hasObjectsList)
	//{
	//	// Parse objects list
	//	const Json::Value& contextObjectsVal = m_jsonRoot[k_contextObjectsKey];
	//	uint64_t masterObjectId = m_jsonRoot[k_contextMasterObjectKey].asUInt64();
	//	Json::Value const* masterObjectVal = nullptr;

	//	// Find master object json val
	//	for (const Json::Value& contextObjectVal : contextObjectsVal)
	//	{
	//		if (contextObjectVal.isMember(k_contextIdKey) && contextObjectVal.isMember(k_contextValKey) && contextObjectVal[k_contextIdKey].asUInt64() == masterObjectId)
	//		{
	//			masterObjectVal = &contextObjectVal;
	//			break;
	//		}
	//	}

	//	if (nullptr != masterObjectVal)
	//	{
	//		// Parse master object
	//		ReadContextObject(type, value, *masterObjectVal);
	//		FilterReferencedObjectsList(m_referencedContextObjects);

	//		while (!m_referencedContextObjects.empty())
	//		{
	//			// Handle referenced context objects
	//			for (const auto& objectReference : m_referencedContextObjects)
	//			{
	//				bool objectAlreadyLoaded = !!m_context->GetObjectById(objectReference.first);
	//				if (!objectAlreadyLoaded)
	//				{
	//					// Find object in context
	//					Json::Value const* contextJsonObjectPtr = FindContextJsonObject(contextObjectsVal, objectReference.first);
	//					if (nullptr != contextJsonObjectPtr)
	//					{
	//						rttr::Type pointedType = objectReference.second;
	//						if (pointedType.IsValid())
	//						{
	//							void* pointedValue = pointedType.Instantiate();
	//							ReadContextObject(pointedType, pointedValue, *contextJsonObjectPtr);
	//						}
	//						else
	//						{
	//							m_context->AddObject(objectReference.first, pointedType, nullptr);
	//						}
	//					}
	//				}
	//			}

	//			FilterReferencedObjectsList(m_referencedContextObjects);
	//		}
	//	}
	//	else
	//	{
	//		Log::LogMessage("Master object not found in the context objects list!");
	//	}
	//}
	//else
	//{
	//	// We have single object, simply read it here
	//	DoRead(type, value);
	//}

	DoRead(type, value);

	// Perform deferred actions
	for (const auto& action : m_deferredCommandsList)
	{
		action->Perform();
	}

	// Release context
	m_context.reset();
}

void BaseReader::FilterReferencedObjectsList(std::vector<std::pair<uint64_t, rttr::Type>>& objectsList)
{
	for (auto it = objectsList.begin(); it != objectsList.end();)
	{
		bool objectLoaded = !!m_context->GetObjectById(it->first);
		if (objectLoaded)
		{
			it = objectsList.erase(it);
		}
		else
		{
			++it;
		}
	}
}

}
