#pragma once

#include "hxcomm/vx/quiggeldy_connection.h"
#include "hxcomm/vx/quiggeldy_utility.h"

#include "slurm/vision_defines.h"

#include <cstdlib>
#include <tuple>

namespace hxcomm::vx {

/**
 * RAII-style helper to ensure a local quiggeldy instance is running.
 */
class EnsureLocalQuiggeldy
{
public:
	EnsureLocalQuiggeldy() : m_quiggeldy_pid{0}
	{
		// If there is no remotely provided quiggeldy instance, we launch one ourselves
		if (std::getenv(vision_quiggeldy_enabled_env_name) == nullptr) {
			std::tie(m_quiggeldy_pid, m_quiggeldy_port) = launch_quiggeldy_locally_from_env();
		}
	}

	~EnsureLocalQuiggeldy()
	{
		if (m_quiggeldy_pid != 0) {
			hxcomm::terminate(m_quiggeldy_pid);
		}
	}

private:
	pid_t m_quiggeldy_pid;
	port_t m_quiggeldy_port;
};

} // namespace hxcomm::vx
