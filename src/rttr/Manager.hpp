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
#include "rttr/GenericTypeSpecializer.hpp"
#include "rttr/StlGenericTypesSpecializers.hpp"
#include "helper/TypeTraitsExtension.hpp"
#include "rs/log/Log.hpp"
#include "rs/SerializationMethod.hpp"

#include <unordered_map>
#include <string>
#include <typeindex>
#include <type_traits>
#include <memory>

namespace rttr
{

template <typename T>
Type Reflect();

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

// Helper function for pointers assignment
void RAVEN_SERIALIZE_API AssignPointerValue(void* pointerAddress, void* value);

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
				ArrayTraitsResolver<T> arrayTraitsResolver;
				metaTypeData.typeParams.array_ = new ArrayParams();
				arrayTraitsResolver(*metaTypeData.typeParams.array_);
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
		default:
			break;
		}

		metaTypeData.debugValueViewer = &DebugValueViewerF<T>;

		// Get class behavior from GenericTypeSpecializer possible specialization
		GenericTypeSpecializer<T> genericTypeSpecializer(metaTypeData);
	}

	Type RAVEN_SERIALIZE_API GetMetaTypeByName(const char* name);
	Type RAVEN_SERIALIZE_API GetMetaTypeByTypeIndex(const std::type_index& typeIndex);

	void RAVEN_SERIALIZE_API RegisterProxyType(const Type& type, const Type& proxyType);
	RAVEN_SERIALIZE_API TypeProxyData* GetProxyType(const Type& type);

	void RAVEN_SERIALIZE_API RegisterSerializationAdapter(const Type& type, std::unique_ptr<rs::SerializationAdapter>&& adapter);
	RAVEN_SERIALIZE_API rs::SerializationAdapter* GetSerializationAdapter(const Type& type);

	rs::SerializationMethod RAVEN_SERIALIZE_API GetSerializationMethod(const Type& type) const;

	static RAVEN_SERIALIZE_API Manager& GetRTTRManager();
	static void RAVEN_SERIALIZE_API InitRTTR();
	static void RAVEN_SERIALIZE_API DestroyRTTR();

private:
	void RAVEN_SERIALIZE_API AddTypeDataInternal(const std::type_index& typeIndex, std::unique_ptr<type_data>&& typeData);

private:
	std::unordered_map<std::type_index, std::unique_ptr<type_data>> m_types;
	std::unordered_map<std::size_t, rs::SerializationMethod> m_customSerializationTypes;
	std::unordered_map<std::string, type_data*> m_typeNames;
	std::unordered_map<Type, std::unique_ptr<TypeProxyData>> m_proxyTypes;
	std::unordered_map<Type, std::unique_ptr<rs::SerializationAdapter>> m_serializationAdapters;
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
