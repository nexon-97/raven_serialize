#pragma once

namespace rs
{

class SerializationKeywords
{
public:
	static const char* TypeId();
	static const char* MasterObjectId();
	static const char* ContextObjects();
	static const char* ContextObjectId();
	static const char* ContextObjectVal();
	static const char* CollectionItems();
};

}

#define K_TYPE_ID rs::SerializationKeywords::TypeId()
#define K_MASTER_OBJ_ID rs::SerializationKeywords::MasterObjectId()
#define K_CONTEXT_OBJECTS rs::SerializationKeywords::ContextObjects()
#define K_CONTEXT_OBJ_ID rs::SerializationKeywords::ContextObjectId()
#define K_CONTEXT_OBJ_VAL rs::SerializationKeywords::ContextObjectVal()
#define K_COLLECTION_ITEMS rs::SerializationKeywords::CollectionItems()
