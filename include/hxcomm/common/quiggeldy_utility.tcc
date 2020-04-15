#include "hxcomm/common/quiggeldy_utility.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <tuple>

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/fpga_ip_list.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/quiggeldy_common.h"

#include "slurm/vision_defines.h"

namespace hxcomm {

/**
 *  Get a free port number to use for tests, on success, return open port.
 *
 *  On error, return 0.
 */
hxcomm::port_t get_unused_port()
{
	struct sockaddr_in sin;
	unsigned int addrlen = sizeof(sin);
	int16_t local_port;

	bzero(&sin, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
		return 0;
	}

	if (bind(sockfd, (struct sockaddr*) &sin, sizeof(sin)) != 0) {
		std::cerr << "Failed to bind socket to loopback: " << strerror(errno) << std::endl;
		return 0;
	}
	if (getsockname(sockfd, (struct sockaddr*) &sin, &addrlen) != 0) {
		std::cerr << "Failed to get bound address for socket: " << strerror(errno) << std::endl;
		return 0;
	}

	local_port = ntohs(sin.sin_port);

	close(sockfd);
	return local_port;
}

void terminate(pid_t pid)
{
	int status;

	kill(pid, SIGTERM);
	waitpid(pid, &status, 0); // wait for the child to exit

	if (!(WIFSIGNALED(status) && (WTERMSIG(status) == SIGTERM))) {
		auto log = log4cxx::Logger::getLogger("hxcomm::terminate");
		HXCOMM_LOG_ERROR(log, "Error terminating PID: " << pid);
	}
}

namespace detail {

template <typename...>
struct all_char_const_ptr;

template <>
struct all_char_const_ptr<> : std::true_type
{};

template <typename Head, typename... Tail>
struct all_char_const_ptr<Head, Tail...>
    : std::integral_constant<
          bool,
          std::is_convertible<Head, char const*>::value && all_char_const_ptr<Tail...>::value>
{};

} // namespace detail


template <class... Args>
pid_t setup_quiggeldy(char const* binary_name, uint16_t port, Args... args)
{
	static_assert(
	    detail::all_char_const_ptr<Args...>::value,
	    "All arguments need to be C-Strings to pass to execlp!");

	pid_t pid = fork();
	if (pid == 0) {
		std::stringstream port_ss;
		port_ss << port;
		auto log = log4cxx::Logger::getLogger("hxcomm.test.util.quiggeldy.setup");

		if (log->isEnabledFor(log4cxx::Level::getDebug())) {
			std::stringstream message;
			message << "Executing: "
			        << "quiggeldy "
			        << "-p " << port_ss.str();
			((message << ' ' << std::forward<Args>(args)), ...);

			HXCOMM_LOG_DEBUG(log, message.str());
		}
		execlp(binary_name, binary_name, "-p", port_ss.str().c_str(), args..., NULL);
		exit(0);
	} else {
		return pid;
	}
}

} // namespace hxcomm
