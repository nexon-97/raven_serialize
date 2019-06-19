#include "rttr/Manager.hpp"

namespace rttr
{

Manager& Manager::GetRTTRManager()
{
	static Manager s_manager;
	return s_manager;
}

Type Manager::GetMetaTypeByName(const char* name)
{
	auto it = m_typeNames.find(name);
	if (it != m_typeNames.end())
	{
		return Type(it->second);
	}

	return Type(nullptr);
}

Type Manager::GetMetaTypeByTypeIndex(const std::type_index& typeIndex)
{
	auto it = m_types.find(typeIndex);
	if (it != m_types.end())
	{
		return Type(&it->second);
	}

	return Type(nullptr);
}

Type Reflect(const char* name)
{
	return Manager::GetRTTRManager().GetMetaTypeByName(name);
}

Type Reflect(const std::type_index& typeIndex)
{
	return Manager::GetRTTRManager().GetMetaTypeByTypeIndex(typeIndex);
}

} // namespace rttr
