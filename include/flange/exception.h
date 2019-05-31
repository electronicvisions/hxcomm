#pragma once

#include <exception>
#include <string>

namespace flange {

class timeout : public virtual std::exception
{
public:
	explicit timeout(std::string const& msg);

	virtual ~timeout() noexcept override = default;

	virtual const char* what() const noexcept override;

private:
	std::string const m_msg;
};

} // namespace flange
