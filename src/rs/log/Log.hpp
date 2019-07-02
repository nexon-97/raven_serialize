#pragma once
#include "rs/log/ILogger.hpp"
#include "raven_serialize.hpp"

#include <vector>

namespace rs
{

class Log
{
public:
	static void RAVEN_SER_API AddLogger(ILogger* logger);
	static void RAVEN_SER_API LogMessage(std::string format, ...);
	static void RAVEN_SER_API Enable(const bool enable);

private:
	static std::vector<ILogger*> s_loggers;
	static bool s_isEnabled;
};

} // namespace rs
