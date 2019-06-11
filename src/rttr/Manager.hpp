#pragma once
#include "rttr/Class.hpp"

#include <unordered_map>
#include <string>

namespace rttr
{

class Manager
{
public:
	Manager() = default;

private:
	std::unordered_map<std::string, Class> m_classes;
};

} // namespace rttr
