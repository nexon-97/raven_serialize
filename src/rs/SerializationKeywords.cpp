#include "rs/SerializationKeywords.hpp"

namespace rs
{

const char* SerializationKeywords::TypeId()
{
	return "$type$";
}

const char* SerializationKeywords::MasterObjectId()
{
	return "$master_obj$";
}

const char* SerializationKeywords::ContextObjects()
{
	return "$objects$";
}

const char* SerializationKeywords::ContextObjectId()
{
	return "$id$";
}

const char* SerializationKeywords::ContextObjectVal()
{
	return "$val$";
}

const char* SerializationKeywords::CollectionItems()
{
	return "$items$";
}

const char* SerializationKeywords::Bases()
{
	return "$bases$";
}

const char* SerializationKeywords::BaseId()
{
	return "$base$";
}

const char* SerializationKeywords::AdapterData()
{
	return "$adapter$";
}

}
