#pragma once

#include <unistd.h>
#include <arpa/inet.h>

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/logger.h"


namespace hxcomm {

/**
 *  Get a free port number to use for tests, on success, return open port.
 *
 *  On error, return 0.
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
 * @tparam Args The additional arguments to give to quiggeldy (should be C-strings).
 *
 * @return PID of started subprocess running quiggeldy.
 */
template <class... Args>
pid_t setup_quiggeldy(char const* binary_name, uint16_t port, Args... args);

} // namespace hxcomm::test::quiggeldy

#include "hxcomm/common/quiggeldy_utility.tcc"
