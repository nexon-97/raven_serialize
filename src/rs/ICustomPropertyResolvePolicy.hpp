#pragma once
#include "rttr/Type.hpp"

namespace rs
{

class ICustomPropertyResolvePolicy
{
public:
	struct SerializeResult
	{
		rttr::Type valueType;
		void* value = nullptr;
		bool needRelease = false;

		SerializeResult(const rttr::Type& type, void* value, bool needRelease)
			: valueType(type)
			, value(value)
			, needRelease(needRelease)
		{}
	};

	// Given object pointer, generate data for serialization
	virtual SerializeResult Serialize(const void* object) = 0;

	// Given object pointer, and pointer to deserialized data, process the data and pass it to the target object
	virtual void Deserialize(void* object, const rttr::Type& dataType, void* dataPtr) = 0;

	// Define primary type of property
	virtual rttr::Type GetType() const = 0;
};

} // namespace rs
