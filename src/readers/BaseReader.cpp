#include "readers/BaseReader.hpp"
#include "rs/log/Log.hpp"

namespace rs
{

void BaseReader::Read(const rttr::Type& type, void* value)
{
	// Create serialization context so reader implementations will be able to use it
	m_context = std::make_unique<rs::detail::SerializationContext>();

	// Call read operation implementation
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
