#include "hxcomm/common/quiggeldy_utility.h"

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

namespace detail {

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
		auto log = log4cxx::Logger::getLogger("hxcomm.setup_quiggeldy");

		if (log->isEnabledFor(log4cxx::Level::getDebug())) {
			std::stringstream message;
			message << "Executing: "
			        << "\"quiggeldy "
			        << "-p " << port_ss.str();
			((message << ' ' << std::forward<Args>(args)), ...);
			message << "\"";

			HXCOMM_LOG_DEBUG(log, message.str());
		}

		// Debugging output from quiggeldy process
		auto* quiggeldy_test_log = std::getenv("QUIGGELDY_TEST_LOG");
		if (quiggeldy_test_log != nullptr) {
			int fd = open(quiggeldy_test_log, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			dup2(fd, 1);
			dup2(fd, 2);
		}

		int ret = execlp(binary_name, binary_name, "-p", port_ss.str().c_str(), args..., NULL);
		assert(ret < 0);
		if (ret < 0) {
			std::stringstream message;
			message << "Executing quiggeldy failed:" << strerror(errno);
			HXCOMM_LOG_ERROR(log, message.str());
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		std::stringstream ss;
		ss << "Could not launch quiggeldy. Error code: " << pid;
		throw std::runtime_error(ss.str());
	} else {
		if (kill(pid, 0) == 0) {
			return pid;
		} else {
			std::stringstream ss;
			ss << "Could not launch quiggeldy. Error code: " << pid;
			throw std::runtime_error(ss.str());
		}
	}
}

} // namespace hxcomm
