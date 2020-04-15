#pragma once

#include <optional>
#include <unistd.h>
#include <arpa/inet.h>

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/logger.h"


namespace hxcomm {

/**
 *  Get a free port number to use for tests, on success, return open port.
 *
 *  On error, throw runtime_error.
 */
inline hxcomm::port_t get_unused_port();

/**
 * Terminate given pid.
 *
 * @param pid The quiggeldy PID to terminate.
 */
inline void terminate(pid_t pid);

/**
 * Convenience function to setup quiggeldy under the given port with additional
 * arguments.
 *
 * @param binary_name name of the binary to launch
 * @param port The port on which quiggeldy should listen.
 * @param args Additional arguments to give to quiggeldy (should be C-strings).
 *
 * @return PID of started subprocess running quiggeldy.
 */
template <class... Args>
pid_t setup_quiggeldy(char const* binary_name, uint16_t port, Args... args);

/**
 * Get an integer representation of the loglevel variable from environment.
 *
 * If the environmental variable is defined but contains an invalid value, a
 * notification is printed to stderr.
 *
 * @param env_var Name of the environment variable from which to get the loglevel.
 * @return Integer representation of the loglevel in case it is defined and a valid level.
 */
inline std::optional<std::size_t> get_loglevel_from_env(char const* env_var = "QUIGGELDY_LOGLEVEL");

} // namespace hxcomm

#include "hxcomm/common/quiggeldy_utility.tcc"
