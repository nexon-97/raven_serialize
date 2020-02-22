#pragma once
#include "rttr/TypeInitContext.hpp"
#include "rttr/Property.hpp"
#include "rttr/TypeProxyData.hpp"
#include "rttr/details/PointerParams.hpp"
#include "rttr/details/ArrayParams.hpp"
#include "rttr/details/EnumParams.hpp"
#include "rttr/details/ObjectClassParams.hpp"
#include "rttr/details/ScalarParams.hpp"
#include "rttr/details/EnumParams.hpp"
#include "helper/TypeTraitsExtension.hpp"
#include "rs/log/Log.hpp"

#include <unordered_map>
#include <string>
#include <typeindex>
#include <type_traits>
#include <memory>

namespace rttr
{

template <typename T>
Type Reflect();

//template <typename T, std::size_t ...Is>
//void FillArrayExtentImpl(std::size_t* arrayExtents, std::index_sequence<Is...>)
//{
//	((arrayExtents[std::integral_constant<std::size_t, Is>{}] = std::extent<T, Is>::value), ...);
//}
//
//template <typename T>
//void FillArrayExtent(std::size_t* arrayExtents)
//{
//	FillArrayExtentImpl<T>(arrayExtents, std::make_index_sequence<std::rank<T>::value>());
//}

template <typename T, typename Cond = void>
struct DefaultInstanceAllocator
{
	void* operator()() const
	{
		return nullptr;
	}
};

template <typename T>
struct DefaultInstanceAllocator<T, std::enable_if_t<std::is_default_constructible<T>::value>>
{
	void* operator()() const
	{
		return reinterpret_cast<void*>(new T());
	}
};

template <typename T>
struct DefaultInstanceAllocator<const T, std::enable_if_t<std::is_default_constructible<const T>::value>>
{
	void* operator()() const
	{
		return const_cast<void*>(reinterpret_cast<const void*>(new T()));
	}
};

template <typename T>
struct DefaultInstanceDestructor
{
	void operator()(void* object) const
	{
		T* objectPtr = reinterpret_cast<T*>(object);
		delete objectPtr;
	}
};

template <typename T, std::size_t N>
struct DefaultInstanceDestructor<T[N]>
{
	using UnderlyingType = typename std::remove_extent<T>::type;

	void operator()(void* object) const
	{
		UnderlyingType* arrayPtr = reinterpret_cast<UnderlyingType*>(object);
		delete[] arrayPtr;
	}
};

//template <typename T>
//struct SmartPtrRawValueResolver
//{
//	void* operator()(void* value)
//	{
//		return nullptr;
//	}
//};
//
//template <typename T>
//struct SmartPtrRawValueResolver<std::shared_ptr<T>>
//{
//	void* operator()(void* value)
//	{
//		std::shared_ptr<T>* smartptr = reinterpret_cast<std::shared_ptr<T>*>(value);
//		return smartptr->get();
//	}
//};
//
//template <typename T>
//struct SmartPtrRawValueResolver<std::unique_ptr<T>>
//{
//	void* operator()(void* value)
//	{
//		std::unique_ptr<T>* smartptr = reinterpret_cast<std::unique_ptr<T>*>(value);
//		return smartptr->get();
//	}
//};
//
//template <typename T, typename Cond = void>
//struct ArrayDataResolver
//{
//	ArrayDataResolver(type_data& metaTypeData) {}
//};
//
//template <typename T>
//void StlCollectionResize(void* collectionPtr, const std::size_t size)
//{
//	T* collection = reinterpret_cast<T*>(collectionPtr);
//	collection->resize(size);
//}
//
//template <typename T>
//std::size_t GetStlCollectionSize(const void* collectionPtr)
//{
//	const T* collection = reinterpret_cast<const T*>(collectionPtr);
//	return collection->size();
//}
//
//template <typename T>
//void* GetStlCollectionItem(const void* collectionPtr, const std::size_t idx)
//{
//	const T* collection = reinterpret_cast<const T*>(collectionPtr);
//	return const_cast<void*>(reinterpret_cast<const void*>(collection->data() + idx));
//}
//
//void RAVEN_SERIALIZE_API CollectionResizeNoop(void*, const std::size_t);
//
//template <typename T>
//std::size_t GetStaticArraySize(const void* collectionPtr)
//{
//	return std::extent<T>::value;
//}
//
//template <typename T>
//void* GetStaticArrayItem(const void* collectionPtr, const std::size_t idx)
//{
//	using UnderlyingType = typename std::remove_extent<T>::type;
//
//	T* castedCollection = const_cast<T*>(reinterpret_cast<const T*>(collectionPtr));
//	UnderlyingType* item = reinterpret_cast<UnderlyingType*>(castedCollection) + idx;
//
//	return reinterpret_cast<void*>(item);
//}
//
//template <typename T>
//void AssignSmartptrValue(void* smartptr, void* value)
//{
//	T* smartPtrCasted = reinterpret_cast<T*>(smartptr);
//	auto dataPtr = reinterpret_cast<typename std::add_pointer<typename std::pointer_traits<T>::element_type>::type>(value);
//	smartPtrCasted->reset(dataPtr);
//}

//template <typename T>
//struct ArrayDataResolver<T, std::enable_if_t<is_std_vector<T>::value>>
//{
//	ArrayDataResolver(type_data& metaTypeData)
//	{
//		metaTypeData.arrayRank = 1U;
//		metaTypeData.arrayTraits.isStdVector = true;
//		metaTypeData.underlyingType[0] = new Type(Reflect<typename std_vector_type<T>::type>());
//
//		metaTypeData.arrayExtents.reset(new std::size_t[metaTypeData.arrayRank]);
//		FillArrayExtent<T>(metaTypeData.arrayExtents.get());
//
//		// Fill dynamic array params
//		static DynamicArrayParams dynamicArrayParams;
//		dynamicArrayParams.resizeFunc = StlCollectionResize<T>;
//		dynamicArrayParams.getSizeFunc = GetStlCollectionSize<T>;
//		dynamicArrayParams.getItemFunc = GetStlCollectionItem<T>;
//		metaTypeData.dynamicArrayParams = &dynamicArrayParams;
//	}
//};
//
//template <typename T>
//struct ArrayDataResolver<T, std::enable_if_t<std::is_array<T>::value>>
//{
//	ArrayDataResolver(type_data& metaTypeData)
//	{
//		metaTypeData.arrayRank = std::rank<T>::value;
//		metaTypeData.arrayTraits.isSimpleArray = true;
//		metaTypeData.underlyingType[0] = new Type(Reflect<typename std::remove_all_extents_t<T>>());
//
//		metaTypeData.arrayExtents.reset(new std::size_t[metaTypeData.arrayRank]);
//		FillArrayExtent<T>(metaTypeData.arrayExtents.get());
//
//		// Fill dynamic array params
//		static DynamicArrayParams dynamicArrayParams;
//		dynamicArrayParams.resizeFunc = CollectionResizeNoop;
//		dynamicArrayParams.getSizeFunc = GetStaticArraySize<T>;
//		dynamicArrayParams.getItemFunc = GetStaticArrayItem<T>;
//		metaTypeData.dynamicArrayParams = &dynamicArrayParams;
//	}
//};

//template <typename T, typename Cond = void>
//struct SmartPointerTraitsResolver
//{
//	SmartPointerTraitsResolver(type_data& metaTypeData) {}
//};
//
//template <typename T>
//struct SmartPointerTraitsResolver<T, std::enable_if_t<is_smart_ptr<T>::value>>
//{
//	SmartPointerTraitsResolver(type_data& metaTypeData)
//	{
//		static SmartPtrParams smartptrParams;
//		smartptrParams.smartptrTypeName = smart_ptr_type_name_resolver<T>()();
//		smartptrParams.valueAssignFunc = AssignSmartptrValue<T>;
//
//		metaTypeData.underlyingType[0] = new Type(Reflect<typename smart_ptr_type<T>::type>());
//		metaTypeData.smartPtrValueResolver = SmartPtrRawValueResolver<T>();
//		metaTypeData.smartptrParams = &smartptrParams;
//	}
//};

///////////////////////////////////////////////////////////////////////////////////////

template <typename T>
const void* DebugValueViewerF(const void* value)
{
	const T* realValuePtr = reinterpret_cast<const T*>(value);
	return realValuePtr;
};

template <typename Signature>
std::unique_ptr<Property> CreateMemberProperty(const char* name, Signature signature)
{
	using T = typename ExtractClassType<Signature>::type;
	using ValueType = typename ExtractValueType<Signature>::type;

	return std::make_unique<MemberProperty<T, ValueType, Signature>>(name, signature, Reflect<ValueType>());
}

template <typename GetterSignature, typename SetterSignature>
std::unique_ptr<Property> CreateIndirectProperty(const char* name, GetterSignature getter, SetterSignature setter)
{
	static_assert(std::is_same<typename ExtractValueType<GetterSignature>::type, typename ExtractValueType<SetterSignature>::type>::value, "Setter ang getter types mismatch!");
	using ValueType = typename ExtractValueType<GetterSignature>::type;	
	using T = typename ExtractClassType<GetterSignature>::type;

	return std::make_unique<IndirectProperty<T, ValueType, GetterSignature, SetterSignature>>(name, getter, setter, Reflect<ValueType>());
}

////////////////////////////////////////////////////////////////////////////////////

class Manager
{
public:
	Manager() = default;
	~Manager() = default;

	void Init();

	template <typename T, typename AllocatorT>
	TypeInitContext<T> CreateTypeInitContext(const char* name, AllocatorT allocator)
	{
		Type type = RegisterMetaType<T>(name, allocator, true);
		return TypeInitContext<T>(type);
	}

	template <typename T, typename AllocatorT = DefaultInstanceAllocator<T>>
	Type RegisterMetaType(const char* name, AllocatorT allocator, const bool userDefined)
	{
		static_assert(std::is_invocable<AllocatorT>::value, "Instance allocator not invokable!");

		auto it = m_types.find(typeid(T));
		if (it == m_types.end())
		{
			const char* typeName = (nullptr != name) ? name : typeid(T).name();
			std::unique_ptr<type_data> i_metaTypeData = std::make_unique<type_data>(TypeClassResolver<T>()(), typeName, m_nextId, sizeof(T), typeid(T));
			type_data* typeDataRawPtr = i_metaTypeData.get();
			AddTypeDataInternal(typeid(T), std::move(i_metaTypeData));
			++m_nextId;

			m_typeNames.emplace(typeName, typeDataRawPtr);

			FillMetaTypeData<T>(*typeDataRawPtr);
			typeDataRawPtr->instanceAllocator = allocator;
			typeDataRawPtr->instanceDestructor = DefaultInstanceDestructor<T>();
			
			Type typeWrapper(typeDataRawPtr);

			rs::Log::LogMessage(std::string("Meta type registered: ") + typeDataRawPtr->name);

			return typeWrapper;
		}
		else
		{
			type_data* metaTypeDataPtr = it->second.get();

			if (userDefined)
			{
				if (nullptr != name)
				{
					// Remove old registered name from lookup table
					auto nameIt = m_typeNames.find(metaTypeDataPtr->name);
					if (nameIt != m_typeNames.end())
					{
						m_typeNames.erase(nameIt);
					}

					metaTypeDataPtr->name = name;
					m_typeNames.emplace(name, metaTypeDataPtr);
				}

				metaTypeDataPtr->isUserDefined = true;
				metaTypeDataPtr->instanceAllocator = allocator;

				rs::Log::LogMessage(std::string("Meta type registered: ") + metaTypeDataPtr->name);
			}

			return Type(metaTypeDataPtr);
		}
	}

	template <typename T>
	void FillMetaTypeData(type_data& metaTypeData)
	{
		metaTypeData.isConst = std::is_const<T>::value;

		switch (metaTypeData.typeClass)
		{
		case TypeClass::Pointer:
			{
				PointerTraitsResolver<T> pointerTraitsResolver;
				metaTypeData.typeParams.pointer = new PointerParams();
				pointerTraitsResolver(*metaTypeData.typeParams.pointer);
			}
			break;
		case TypeClass::Array:
			{
				metaTypeData.typeParams.array_ = new ArrayParams();
			}
			break;
		case TypeClass::Integral:
			{
				ScalarTraitsResolver<T> scalarTraitsResolver;
				metaTypeData.typeParams.scalar = new ScalarParams();
				scalarTraitsResolver(*metaTypeData.typeParams.scalar);
			}
			break;
		case TypeClass::Real:
			{
				metaTypeData.typeParams.scalar = new ScalarParams();
			}
			break;
		case TypeClass::Object:
			{
				ObjectTraitsResolver<T> objectTraitsResolver;
				metaTypeData.typeParams.object = new ObjectClassParams();
				objectTraitsResolver(*metaTypeData.typeParams.object);
			}
			break;
		case TypeClass::Enum:
			{
				EnumTraitsResolver<T> enumTraitsResolver;
				metaTypeData.typeParams.enum_ = new EnumParams();
				enumTraitsResolver(*metaTypeData.typeParams.enum_);
			}
			break;
		}

		metaTypeData.debugValueViewer = &DebugValueViewerF<T>;
	}

	Type RAVEN_SERIALIZE_API GetMetaTypeByName(const char* name);
	Type RAVEN_SERIALIZE_API GetMetaTypeByTypeIndex(const std::type_index& typeIndex);

	void RAVEN_SERIALIZE_API RegisterProxyType(const Type& type, const Type& proxyType);
	RAVEN_SERIALIZE_API TypeProxyData* GetProxyType(const Type& type);

	static RAVEN_SERIALIZE_API Manager& GetRTTRManager();

private:
	void RAVEN_SERIALIZE_API AddTypeDataInternal(const std::type_index& typeIndex, std::unique_ptr<type_data>&& typeData);

private:
	std::unordered_map<std::type_index, std::unique_ptr<type_data>> m_types;
	std::unordered_map<std::string, type_data*> m_typeNames;
	std::unordered_map<Type, TypeProxyData> m_proxyTypes;
	std::size_t m_nextId = 0U;
};

void RAVEN_SERIALIZE_API InitRavenSerialization();

template <typename T>
TypeInitContext<T> DeclType(const char* name)
{
	return Manager::GetRTTRManager().CreateTypeInitContext<T>(name, DefaultInstanceAllocator<T>());
}

template <typename T, typename Alloc>
TypeInitContext<T> DeclType(const char* name, Alloc allocatorObject)
{
	return Manager::GetRTTRManager().CreateTypeInitContext<T>(name, allocatorObject);
}

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
	return Manager::GetRTTRManager().RegisterMetaType<T>(nullptr, DefaultInstanceAllocator<T>(), false);
}

Type RAVEN_SERIALIZE_API Reflect(const char* name);
Type RAVEN_SERIALIZE_API Reflect(const std::type_index& typeIndex);

} // namespace rttr
