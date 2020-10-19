#pragma once
#include "rttr/Type.hpp"

namespace rs
{

class SerializationAdapter
{
public:
	// Optional payload data (put under K_ADAPTER section of resource)
	struct DataChunk
	{
		rttr::Type type;
		void* value = nullptr;
	};

	// 
	struct AdapterReadOutput
	{
		rttr::Type convertedType;
	};

	// 
	struct AdapterWriteOutput
	{
		DataChunk payload;
		DataChunk value;
	};

	// For conversion stage we provide payload data, and return converted type, then reader will try to treat the same value as returned type
	virtual AdapterReadOutput ReadConvert(const DataChunk& payload) = 0;
	// Reader call read finalize after AdapterReadOutput struct has been processed, value contains data read from source, target is pointer to final data
	virtual void ReadFinalize(void* value, void* target, const AdapterReadOutput& output, const DataChunk& payload) = 0;
	// For write, we generate data chunk and payload
	virtual AdapterWriteOutput Write(const void* value) = 0;
	// Finalize when we no longer need this adapter, and can clean up some temp state
	virtual void WriteFinalize(const void* value) {}
	// Returns adapter payload type
	virtual rttr::Type GetPayloadType() const = 0;
};

}
