#pragma once
#include <string>

namespace rs
{

class ILogger
{
public:
	virtual void Log(const std::string& msg) = 0;
};

} // namespace rs
