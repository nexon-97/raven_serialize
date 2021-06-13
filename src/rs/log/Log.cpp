#include "raven_serialize_export.h"
#include "rs/log/Log.hpp"

#include <memory>
#include <cstdarg>
#include <cstring>

namespace rs
{

std::vector<ILogger*> Log::s_loggers;
bool Log::s_isEnabled = true;

void Log::AddLogger(ILogger* logger)
{
	s_loggers.push_back(logger);
}

void Log::LogMessage(std::string format, ...)
{
	if (!s_isEnabled)
		return;

	std::string msg;

	if (!format.empty())
	{
		int final_n, n = ((int)format.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
		std::unique_ptr<char[]> formatted;
		va_list ap;

		while (1)
		{
			formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
			strncpy(&formatted[0], format.c_str(), n);
			va_start(ap, format);
			final_n = vsnprintf(&formatted[0], n, format.c_str(), ap);
			va_end(ap);
			if (final_n < 0 || final_n >= n)
				n += abs(final_n - n + 1);
			else
				break;
		}

		msg = std::string(formatted.get());
	}

	for (ILogger* logger : s_loggers)
	{
		logger->Log(msg);
	}
}

void Log::Enable(const bool enable)
{
	s_isEnabled = enable;
}

} // namespace rs
