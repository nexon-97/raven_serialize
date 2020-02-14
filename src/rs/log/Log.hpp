#pragma once
#include "rs/log/ILogger.hpp"

#include <vector>

namespace rs
{

class Log
{
public:
	static void RAVEN_SERIALIZE_API AddLogger(ILogger* logger);
	static void RAVEN_SERIALIZE_API LogMessage(std::string format, ...);
	static void RAVEN_SERIALIZE_API Enable(const bool enable);

private:
	static std::vector<ILogger*> s_loggers;
	static bool s_isEnabled;
};

} // namespace rs
