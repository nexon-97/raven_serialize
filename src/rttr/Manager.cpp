#include "rttr/Manager.hpp"

namespace rttr
{

Manager& Manager::GetRTTRManager()
{
	static Manager s_manager;
	return s_manager;
}

} // namespace rttr
