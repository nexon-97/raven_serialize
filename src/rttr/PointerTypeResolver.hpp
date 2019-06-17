#pragma once
#include "rttr/Type.hpp"

namespace rttr
{

/*
* @brief Pointer type resolver is used to convert pointer value to serializable value
*
* Type resolver knows about the pointer address, and its metatype
* ResolveResult structure is used to return resolve result.
* Custom resolvers are free to convert given pointer of any type to any desired representation
*/
class PointerTypeResolver
{
public:
	struct ResolveResult
	{
		Type resolvedType;
		const void* resolvedValue = nullptr;
		bool resolved = false;

		ResolveResult()
			: resolvedType(nullptr)
			, resolved(false)
			, resolvedValue(nullptr)
		{}

		ResolveResult(const Type& resolvedType, const void* resolvedValue)
			: resolvedType(resolvedType)
			, resolved(true)
			, resolvedValue(resolvedValue)
		{}
	};

public:
	virtual ResolveResult Resolve(const Type& ptrType, const void* ptr) = 0;
};

class DefaultPointerTypeResolver
	: public PointerTypeResolver
{
public:
	ResolveResult Resolve(const Type& ptrType, const void* ptr) final
	{
		return ResolveResult();
	}
};

} // namespace rttr
