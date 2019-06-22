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

	// Resolve result with additional context
	template <typename T>
	struct ResolveResultWithContext
		: public ResolveResult
	{
		T context;

		ResolveResultWithContext() = delete;

		explicit ResolveResultWithContext(const Type& resolvedType, const void* resolvedValue, T&& context)
			: ResolveResult(resolvedType, resolvedValue)
			, context(std::move(context))
		{}
	};

public:
	virtual std::unique_ptr<ResolveResult> Resolve(const Type& ptrType, const void* ptr) = 0;
	// [TODO] Remove better names for interface methods
	virtual std::unique_ptr<ResolveResult> ResolveReverse(const Type& ptrType, const Type& dataType, std::uintptr_t* ptr, const void* dataValue) = 0;
};

class DefaultPointerTypeResolver
	: public PointerTypeResolver
{
public:
	std::unique_ptr<ResolveResult> Resolve(const Type& ptrType, const void* ptr) final
	{
		return std::make_unique<ResolveResult>();
	}

	std::unique_ptr<ResolveResult> ResolveReverse(const Type& ptrType, const Type& dataType, std::uintptr_t* ptr, const void* dataValue) final
	{
		return std::make_unique<ResolveResult>();
	}
};

} // namespace rttr
