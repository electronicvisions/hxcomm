#pragma once

#include <exception>
#include <string>

namespace flange {

class timeout : public virtual std::exception
{
public:
	explicit timeout(std::string const& msg) : m_msg(msg) {}

	virtual ~timeout() noexcept override = default;

	virtual const char* what() const noexcept override { return m_msg.c_str(); }

private:
	std::string const m_msg;
};

} // namespace flange
