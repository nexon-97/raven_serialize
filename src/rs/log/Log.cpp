#include "rs/log/Log.hpp"

namespace rs
{

std::vector<ILogger*> Log::s_loggers;

void Log::AddLogger(ILogger* logger)
{
	s_loggers.push_back(logger);
}

void Log::LogMessage(const std::string& msg)
{
	for (ILogger* logger : s_loggers)
	{
		logger->Log(msg);
	}
}

} // namespace rs
