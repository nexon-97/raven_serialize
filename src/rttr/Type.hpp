#pragma once
#include "helper/TypeTraitsExtension.hpp"

#include <unordered_map>
#include <string>
#include <memory>
#include <cassert>

namespace rttr
{

struct type_data
{
	const char* name = nullptr;
	const std::size_t id = 0U;
	const bool isIntegral;
	const bool isFloat;
	const bool isArray;
	const bool isEnum;
	const bool isClass;
	const bool isFunction;
	const bool isPointer;
	const bool isMemberObjPointer;
	const bool isMemberFuncPointer;
	const bool isConst;
	const bool isSigned;
	const std::size_t arrayRank;
	const std::size_t* arrayExtents = nullptr;

	type_data() = default;

	~type_data()
	{
		if (nullptr != arrayExtents)
		{
			delete[] arrayExtents;
		}
	}
};

class Type
{
public:
	Type(const type_data& typeData)
		: m_typeData(typeData)
	{}

	const char* GetName() const;
	const std::size_t GetId() const;
	const bool IsIntegral() const;
	const bool IsFloatingPoint() const;
	const bool IsArray() const;
	const bool IsEnum() const;
	const bool IsClass() const;
	const bool IsFunction() const;
	const bool IsPointer() const;
	const bool IsMemberObjectPointer() const;
	const bool IsMemberFunctionPointer() const;
	const bool IsConst() const;
	const bool IsSigned() const;
	const bool GetArrayRank() const;
	const bool GetArrayExtent(const std::size_t dimension = 0U) const;

private:
	type_data m_typeData;
};

} // namespace rttr
