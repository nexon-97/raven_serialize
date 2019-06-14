#pragma once
#include "rttr/Type.hpp"
#include "raven_serialize.hpp"

#include <unordered_map>
#include <string>
#include <typeindex>
#include <memory>

namespace rttr
{

template <typename T, std::size_t ...Is>
void FillArrayExtentImpl(std::size_t* arrayExtents, std::index_sequence<Is...>)
{
	((arrayExtents[std::integral_constant<std::size_t, Is>{}] = std::extent<T, Is>::value), ...);
}

template <typename T>
void FillArrayExtent(std::size_t* arrayExtents)
{
	FillArrayExtentImpl<T>(arrayExtents, std::make_index_sequence<std::rank<T>::value>());
}

template <typename T>
auto DefaultInstanceAllocator = []() -> void*
{
	return reinterpret_cast<void*>(new T());
};

class Manager
{
public:
	Manager() = default;
	~Manager() = default;

	template <typename T>
	Type RegisterMetaType(const char* name = nullptr, const MetaTypeInstanceAllocator& instanceAllocator = DefaultInstanceAllocator<T>)
	{
		auto it = m_types.find(typeid(T));
		if (it == m_types.end())
		{
			const char* typeName = (nullptr != name) ? name : typeid(T).name();
			type_data i_metaTypeData(typeName, m_nextId, sizeof(T), typeid(T));
			++m_nextId;

			auto emplaceResult = m_types.emplace(typeid(T), i_metaTypeData);
			type_data& metaTypeData = emplaceResult.first->second;

			metaTypeData.isIntegral = std::is_integral<T>::value;
			metaTypeData.isFloat = std::is_floating_point<T>::value;
			metaTypeData.isArray = std::is_array<T>::value || is_std_vector<T>::value || is_std_array<T>::value;
			metaTypeData.isEnum = std::is_enum<T>::value;
			metaTypeData.isClass = std::is_class<T>::value;
			metaTypeData.isFunction = std::is_function<T>::value;
			metaTypeData.isPointer = std::is_pointer<T>::value;
			metaTypeData.isMemberObjPointer = std::is_member_object_pointer<T>::value;
			metaTypeData.isMemberFuncPointer = std::is_member_function_pointer<T>::value;
			metaTypeData.isConst = std::is_const<T>::value;
			metaTypeData.isSigned = std::is_signed<T>::value;
			metaTypeData.isString = is_string<T>::value;
			metaTypeData.instanceAllocator = instanceAllocator;

			if (is_std_vector<T>::value)
			{
				metaTypeData.arrayRank = 1U;
				metaTypeData.arrayTraits.isStdArray = true;
				metaTypeData.underlyingType[0] = new Type(Reflect<std_vector_type<T>::type>());
			}
			else
			{
				metaTypeData.arrayRank = std::rank<T>::value;
				metaTypeData.arrayTraits.isSimpleArray = true;
				metaTypeData.underlyingType[0] = new Type(Reflect<std::remove_all_extents_t<T>>());
			}

			if (metaTypeData.isArray)
			{
				metaTypeData.arrayExtents.reset(new std::size_t[metaTypeData.arrayRank]);
				FillArrayExtent<T>(metaTypeData.arrayExtents.get());
			}

			Type typeWrapper(&(emplaceResult.first->second));

			return typeWrapper;
		}
		else
		{
			return Type(&(it->second));
		}
	}

	static RAVEN_SER_API Manager& GetRTTRManager();

private:
	std::unordered_map<std::type_index, type_data> m_types;
	std::size_t m_nextId = 0U;
};

template <typename T>
Type MetaType(const char* name)
{
	static_assert(std::is_default_constructible<T>::value, "Metatypes must be default constructible!");
	return Manager::GetRTTRManager().RegisterMetaType<T>(name);
}

template <typename T>
Type Reflect()
{
	static_assert(std::is_default_constructible<T>::value, "Metatypes must be default constructible!");
	return Manager::GetRTTRManager().RegisterMetaType<T>();
}

} // namespace rttr
