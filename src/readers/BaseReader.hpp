#pragma once
#include "readers/IReader.hpp"
#include "rttr/Type.hpp"
#include "rttr/CustomTypeResolver.hpp"
#include "actions/IReaderAction.hpp"
#include "SerializationContext.hpp"
#include "ContextPath.hpp"

#include <istream>
#include <unordered_map>
#include <json/json.h>

namespace rttr
{
	template <typename T>
	Type Reflect();
}

namespace rs
{

class BaseReader
	: public IReader
{
public:
	void RAVEN_SERIALIZE_API Read(const rttr::Type& type, void* value) final;
	void AddCustomTypeResolver(const rttr::Type& type, rttr::CustomTypeResolver* resolver) override;

protected:
	// Removes data about objects that had already been loaded
	void FilterReferencedObjectsList(std::vector<std::pair<uint64_t, rttr::Type>>& objectsList);

	virtual void DoRead(const rttr::Type& type, void* value) = 0;
	virtual bool CheckSourceHasObjectsList() = 0;

protected:
	std::unordered_map<std::type_index, rttr::CustomTypeResolver*> m_customTypeResolvers;
	std::unique_ptr<rs::detail::SerializationContext> m_context;
	std::vector<std::pair<uint64_t, rttr::Type>> m_referencedContextObjects;
	std::vector<std::unique_ptr<detail::IReaderAction>> m_deferredCommandsList;
	std::vector<std::unique_ptr<rttr::CollectionInserterBase>> m_collectionInserters;
	bool m_hasObjectsList = false;
};

} // namespace rs
