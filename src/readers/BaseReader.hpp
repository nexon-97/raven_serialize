#pragma once
#include "readers/IReader.hpp"
#include "rttr/Type.hpp"
#include "actions/IReaderAction.hpp"
#include "SerializationContext.hpp"
#include "ContextPath.hpp"

#include <istream>
#include <unordered_map>
#include <json/json.h>

namespace rs
{

/*
* @brief BaseReader implements basic infrastructure needed to read all supported types of data
*/
class BaseReader
	: public IReader
{
public:
	void RAVEN_SERIALIZE_API Read(const rttr::Type& type, void* value) final;

protected:
	// Removes data about objects that had already been loaded
	void FilterReferencedObjectsList(std::vector<std::pair<uint64_t, rttr::Type>>& objectsList);

	virtual void DoRead(const rttr::Type& type, void* value) = 0;
	virtual bool CheckSourceHasObjectsList() = 0;

protected:
	// Context is used to store the temp variables and objects ids mappings
	std::unique_ptr<rs::detail::SerializationContext> m_context;
	std::vector<std::pair<uint64_t, rttr::Type>> m_referencedContextObjects;
	// Deferred commands list is used to handle complex nested cases, when we read some deep property, up the tree there might be temp variables,
	// and indirect properties, so to make sure everything will be in place, we remember operations, and after we execute them to get final result
	std::vector<std::unique_ptr<detail::IReaderAction>> m_deferredCommandsList;
	bool m_hasObjectsList = false;
};

} // namespace rs
