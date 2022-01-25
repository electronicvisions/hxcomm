#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include <unistd.h>

namespace hxcomm::vx {

/**
 * RAII-style helper to ensure a local quiggeldy instance is running.
 */
class EnsureLocalQuiggeldy
{
public:
	EnsureLocalQuiggeldy() SYMBOL_VISIBLE;
	~EnsureLocalQuiggeldy() SYMBOL_VISIBLE;

private:
	pid_t m_quiggeldy_pid;
	port_t m_quiggeldy_port;
};

} // namespace hxcomm::vx
