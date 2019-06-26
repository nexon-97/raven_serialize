#pragma once
#include "rttr/Type.hpp"
#include "helper/TypeTraitsExtension.hpp"
#include "raven_serialize.hpp"

#include <unordered_map>
#include <string>
#include <typeindex>
#include <type_traits>
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
struct DefaultInstanceAllocator
{
	void* operator()() const
	{
		static_assert(std::is_default_constructible<T>::value, "Metatype must be default constructible!");
		return reinterpret_cast<void*>(new T());
	}
};

struct PlaceholderInstanceAllocator
{
	void* operator()() const
	{
		return nullptr;
	}
};

template <typename T>
std::type_index GetTypeIndexFromPointerType(void* value)
{
	static_assert(std::is_pointer<T>::value, "Can't get type index from pointer type");

	T ptrValue = *reinterpret_cast<T*>(value);
	if (nullptr != ptrValue)
	{
		return typeid(*ptrValue);
	}
	
	return typeid(std::remove_pointer<T>::type);
}

template <typename T>
struct SmartPtrRawValueResolver
{
	void* operator()(void* value)
	{
		return nullptr;
	}
};

template <typename T>
struct SmartPtrRawValueResolver<std::shared_ptr<T>>
{
	void* operator()(void* value)
	{
		std::shared_ptr<T>* smartptr = reinterpret_cast<std::shared_ptr<T>*>(value);
		return smartptr->get();
	}
};

template <typename T>
struct SmartPtrRawValueResolver<std::unique_ptr<T>>
{
	void* operator()(void* value)
	{
		std::unique_ptr<T>* smartptr = reinterpret_cast<std::unique_ptr<T>*>(value);
		return smartptr->get();
	}
};

template <typename T, typename Cond = void>
struct ArrayDataResolver
{
	ArrayDataResolver(type_data& metaTypeData) {}
};

template <typename T>
struct ArrayDataResolver<T, std::enable_if_t<is_std_vector<T>::value>>
{
	ArrayDataResolver(type_data& metaTypeData)
	{
		metaTypeData.arrayRank = 1U;
		metaTypeData.arrayTraits.isStdArray = true;
		metaTypeData.underlyingType[0] = new Type(Reflect<std_vector_type<T>::type>());

		metaTypeData.arrayExtents.reset(new std::size_t[metaTypeData.arrayRank]);
		FillArrayExtent<T>(metaTypeData.arrayExtents.get());
	}
};

template <typename T>
struct ArrayDataResolver<T, std::enable_if_t<std::is_array<T>::value>>
{
	ArrayDataResolver(type_data& metaTypeData)
	{
		metaTypeData.arrayRank = std::rank<T>::value;
		metaTypeData.arrayTraits.isSimpleArray = true;
		metaTypeData.underlyingType[0] = new Type(Reflect<std::remove_all_extents_t<T>>());

		metaTypeData.arrayExtents.reset(new std::size_t[metaTypeData.arrayRank]);
		FillArrayExtent<T>(metaTypeData.arrayExtents.get());
	}
};

template <typename T, typename Cond = void>
struct SmartPointerTraitsResolver
{
	SmartPointerTraitsResolver(type_data& metaTypeData) {}
};

template <typename T>
struct SmartPointerTraitsResolver<T, std::enable_if_t<is_smart_ptr<T>::value>>
{
	SmartPointerTraitsResolver(type_data& metaTypeData)
	{
		static SmartPtrParams smartptrParams;
		smartptrParams.smartptrTypeName = smart_ptr_type_name_resolver<T>()();

		metaTypeData.underlyingType[0] = new Type(Reflect<smart_ptr_type<T>::type>());
		metaTypeData.smartPtrValueResolver = SmartPtrRawValueResolver<T>();
		metaTypeData.smartptrParams = &smartptrParams;
	}
};

template <typename T, typename Cond = void>
struct PointerTraitsResolver
{
	PointerTraitsResolver(type_data& metaTypeData) {}
};

template <typename T>
struct PointerTraitsResolver<T, std::enable_if_t<std::is_pointer<T>::value>>
{
	PointerTraitsResolver(type_data& metaTypeData)
	{
		metaTypeData.underlyingType[0] = new Type(Reflect<std::pointer_traits<T>::element_type>());
		metaTypeData.pointerTypeIndexResolverFunc = GetTypeIndexFromPointerType<T>;
	}
};

template <typename T, typename Cond = void>
struct EnumTraitsResolver
{
	EnumTraitsResolver(type_data& metaTypeData) {}
};

template <typename T>
struct EnumTraitsResolver<T, std::enable_if_t<std::is_enum<T>::value>>
{
	EnumTraitsResolver(type_data& metaTypeData)
	{
		metaTypeData.underlyingType[0] = new Type(Reflect<std::underlying_type<T>::type>());
	}
};

class Manager
{
public:
	Manager() = default;
	~Manager() = default;

	void Init();

	template <typename T, typename AllocatorT = DefaultInstanceAllocator<T>>
	Type RegisterMetaType(const char* name, AllocatorT allocator, const bool userDefined)
	{
		static_assert(std::is_invocable<AllocatorT>::value, "Instance allocator not invokable!");

		auto it = m_types.find(typeid(T));
		if (it == m_types.end())
		{
			const char* typeName = (nullptr != name) ? name : typeid(T).name();
			type_data i_metaTypeData(typeName, m_nextId, sizeof(T), typeid(T));
			++m_nextId;

			auto emplaceResult = m_types.emplace(typeid(T), i_metaTypeData);
			type_data& metaTypeData = emplaceResult.first->second;
			m_typeNames.emplace(typeName, &metaTypeData);

			FillMetaTypeData<T>(metaTypeData);

			if (!std::is_same<AllocatorT, PlaceholderInstanceAllocator>::value)
			{
				metaTypeData.instanceAllocator = allocator;
			}
			
			Type typeWrapper(&(emplaceResult.first->second));

			return typeWrapper;
		}
		else
		{
			if (userDefined)
			{
				auto& metaTypeData = it->second;

				if (nullptr != name)
				{
					metaTypeData.name = name;
				}

				if (!metaTypeData.isUserDefined)
				{
					metaTypeData.isUserDefined = true;
				}

				if (!std::is_same<AllocatorT, PlaceholderInstanceAllocator>::value)
				{
					metaTypeData.instanceAllocator = allocator;
				}
			}

			return Type(&(it->second));
		}
	}

	template <typename T>
	void FillMetaTypeData(type_data& metaTypeData)
	{
		metaTypeData.isIntegral = std::is_integral<T>::value;
		metaTypeData.isFloat = std::is_floating_point<T>::value;
		metaTypeData.isArray = std::is_array<T>::value || is_std_vector<T>::value || is_std_array<T>::value;
		metaTypeData.isEnum = std::is_enum<T>::value;
		metaTypeData.isClass = std::is_class<T>::value;
		metaTypeData.isFunction = std::is_function<T>::value;
		metaTypeData.isPointer = std::is_pointer<T>::value;
		metaTypeData.isSmartPointer = is_smart_ptr<T>::value;
		metaTypeData.isMemberObjPointer = std::is_member_object_pointer<T>::value;
		metaTypeData.isMemberFuncPointer = std::is_member_function_pointer<T>::value;
		metaTypeData.isConst = std::is_const<T>::value;
		metaTypeData.isSigned = std::is_signed<T>::value;
		metaTypeData.isString = is_string<T>::value;

		if (metaTypeData.isArray)
		{
			ArrayDataResolver<T> arrayDataResolver(metaTypeData);
		}

		if (metaTypeData.isPointer)
		{
			PointerTraitsResolver<T> pointerTraitsResolver(metaTypeData);
		}

		if (metaTypeData.isEnum)
		{
			EnumTraitsResolver<T> enumTraitsResolver(metaTypeData);
		}

		if (metaTypeData.isSmartPointer)
		{
			SmartPointerTraitsResolver<T> smartptrTraitsResolver(metaTypeData);
		}
	}

	Type RAVEN_SER_API GetMetaTypeByName(const char* name);
	Type RAVEN_SER_API GetMetaTypeByTypeIndex(const std::type_index& typeIndex);

	static RAVEN_SER_API Manager& GetRTTRManager();

private:
	std::unordered_map<std::type_index, type_data> m_types;
	std::unordered_map<std::string, type_data*> m_typeNames;
	std::size_t m_nextId = 0U;
};

void RAVEN_SER_API InitRavenSerialization();

template <typename T>
Type MetaType(const char* name)
{
	return Manager::GetRTTRManager().RegisterMetaType<T>(name, DefaultInstanceAllocator<T>(), true);
}

template <typename T, typename Alloc>
Type MetaType(const char* name, Alloc allocatorObject)
{
	return Manager::GetRTTRManager().RegisterMetaType<T, Alloc>(name, allocatorObject, true);
}

template <typename T>
Type Reflect()
{
	return Manager::GetRTTRManager().RegisterMetaType<T>(nullptr, PlaceholderInstanceAllocator(), false);
}

Type RAVEN_SER_API Reflect(const char* name);
Type RAVEN_SER_API Reflect(const std::type_index& typeIndex);

} // namespace rttr
