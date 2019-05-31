#include "flange/exception.h"

namespace flange {

timeout::timeout(std::string const& msg) : m_msg(msg) {}

const char* timeout::what() const noexcept
{
	return m_msg.c_str();
}

} // namespace flange
