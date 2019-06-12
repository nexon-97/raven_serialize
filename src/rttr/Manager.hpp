#pragma once
#include "rttr/Class.hpp"
#include "rttr/Type.hpp"

#include <unordered_map>
#include <string>
#include <typeindex>
#include <memory>

namespace rttr
{

class ClassBase;

class Manager
{
public:
	Manager() = default;
	~Manager() = default;

	template <typename T>
	Class<T>& RegisterClass(const std::string& name)
	{
		auto it = m_classes.find(typeid(T));
		if (it == m_classes.end())
		{
			auto emplaceResult = m_classes.emplace(typeid(T), std::make_unique<Class<T>>(name));
			return *static_cast<Class<T>*>(emplaceResult.first->second.get());
		}
		else
		{
			return *static_cast<Class<T>*>(it->second.get());
		}
	}

	template <typename T>
	Type& RegisterMetaType(const char* name)
	{
		type_data metaTypeData;
		metaTypeData.name = name;
		metaTypeData.id = m_nextId;

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
		metaTypeData.arrayRank = std::rank<T>::value;

		if (metaTypeData.isArray)
		{
			metaTypeData.arrayExtents = new std::size_t[metaTypeData.arrayRank];
			for (int i = 0; i < metaTypeData.arrayRank; ++i)
			{
				metaTypeData.arrayExtents[i] = std::extent<T, i>::value;
			}
		}

		++m_nextId;

		auto emplaceResult = m_types.emplace(typeid(T), Type(metaTypeData));
		auto& typeWrapper = emplaceResult.first->second;

		return typeWrapper;
	}

	template <typename T>
	Class<T>* GetClass() const
	{
		auto it = m_classes.find(typeid(T));
		if (it != m_classes.end())
		{
			return static_cast<Class<T>*>(it->second.get());
		}

		return nullptr;
	}

	ClassBase* GetClass(const std::type_index& typeIndex) const
	{
		auto it = m_classes.find(typeIndex);
		if (it != m_classes.end())
		{
			return it->second.get();
		}

		return nullptr;
	}

	static Manager& GetRTTRManager()
	{
		static Manager s_manager;
		return s_manager;
	}

private:
	std::unordered_map<std::type_index, std::unique_ptr<ClassBase>> m_classes;
	std::unordered_map<std::type_index, Type> m_types;
	std::size_t m_nextId = 0U;
};

template <typename T>
Class<T>& DeclClass(const std::string& name)
{
	return Manager::GetRTTRManager().RegisterClass<T>(name);
}

template <typename T>
Type& MetaType(const std::string& name)
{
	return Manager::GetRTTRManager().RegisterMetaType<T>(name);
}

} // namespace rttr
