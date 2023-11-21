#include "hxcomm/common/quiggeldy_utility.h"

#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <tuple>

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
 *  On error, throws runtime_error.
 */
hxcomm::port_t get_unused_port()
{
	struct sockaddr_in sin;
	unsigned int addrlen = sizeof(sin);
	bzero(&sin, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	bool found_port = false;
	auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
	} else if (bind(sockfd, (struct sockaddr*) &sin, sizeof(sin)) != 0) {
		std::cerr << "Failed to bind socket to loopback: " << strerror(errno) << std::endl;
	} else if (getsockname(sockfd, (struct sockaddr*) &sin, &addrlen) != 0) {
		std::cerr << "Failed to get bound address for socket: " << strerror(errno) << std::endl;
	} else {
		// All error cases handled.
		found_port = true;
	}

	if (!found_port) {
		throw std::runtime_error("Failed to determine unused port.");
	}

	int16_t local_port = ntohs(sin.sin_port);

	close(sockfd);
	return local_port;
}

void terminate(pid_t pid)
{
	int status;

	kill(pid, SIGTERM);
	waitpid(pid, &status, 0); // wait for the child to exit

	auto log = log4cxx::Logger::getLogger("hxcomm.terminate");

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) != 0) {
			HXCOMM_LOG_ERROR(
			    log, "Error terminating PID: " << pid << ". Exit status: " << WEXITSTATUS(status));
		}
	} else if (WIFSIGNALED(status)) {
		HXCOMM_LOG_ERROR(
		    log, "Error terminating PID: " << pid << ". Process was terminated by signal: "
		                                   << WTERMSIG(status) << "(" << strsignal(WTERMSIG(status))
		                                   << ")");
	} else {
		HXCOMM_LOG_ERROR(log, "Error terminating PID: " << pid << ". Waitpid returned: " << status);
	}
}

std::optional<std::size_t> get_loglevel_from_env(char const* env_var)
{
	auto env_loglevel = std::getenv(env_var);

	if (env_loglevel == nullptr) {
		return std::nullopt;
	} else {
		std::string_view loglevel(env_loglevel);
		if (std::all_of(loglevel.begin(), loglevel.end(), [](auto& c) -> bool {
			    return std::isdigit(c);
		    })) {
			return std::atoi(env_loglevel);
		} else {
			if (loglevel == "trace") {
				return 0;
			} else if (loglevel == "debug") {
				return 1;
			} else if (loglevel == "info") {
				return 2;
			} else if (loglevel == "warning") {
				return 3;
			} else if (loglevel == "error") {
				return 4;
			} else if (loglevel == "fatal") {
				return 5;
			} else {
				std::cerr << env_var
				          << " option has to be one of "
				             "{trace,debug,info,warning,error,fatal}"
				          << std::endl;
				return std::nullopt;
			}
		}
	}
}

} // namespace hxcomm
