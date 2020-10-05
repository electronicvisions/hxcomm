
#include "hxcomm/common/ensure_local_quiggeldy.h"
#include "hxcomm/common/quiggeldy_utility.h"

#include "logging_ctrl.h"

#include <RCF/RCF.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include <unistd.h>

bool help_requested(int argc, char* argv[])
{
	if (argc != 2) {
		return false;
	} else {
		return (std::strcmp(argv[1], "-h") == 0) || (std::strcmp(argv[1], "--help") == 0);
	}
}

void print_usage(char* argv[])
{
	std::cerr << "Usage: " << argv[0]
	          << " EXECUTABLE [ARGS...]\n\n"
	             "    Ensure a local quiggeldy is running (and set in env) prior to running the\n"
	             "    given executable with the provided arguments.\n\n"
	             "    Any arguments provided prior to the binary name will be forwarded to\n"
	             "    quiggeldy instance. See `quiggeldy --help` for details.\n\n"
	             "    Additionally, the arguments provided to quiggeldy can be cut short early by\n"
	             "    specifying `--`.\n\n"
	          << std::endl;
}

/**
 * Collect all arguments for quiggeldy. Stop at first non-option or early if
 * encountering `--`.
 *
 * @param argv Original argv provided to executable.
 * @return Collected arguments.
 */
std::vector<std::string> collect_args_for_quiggeldy(char* argv[])
{
	std::vector<std::string> collected{};

	for (std::size_t idx = 0; argv[idx] != nullptr; ++idx) {
		if (argv[idx][0] == '-') {
			if (argv[idx][1] == '-' && argv[idx][2] == '\0') {
				break;
			} else {
				collected.push_back(std::string{argv[idx]});
			}
		} else {
			break;
		}
	}
	return collected;
}

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		print_usage(argv);
		std::exit(1);
	} else if (help_requested(argc, argv)) {
		print_usage(argv);
		std::exit(0);
	}

	int loglevel = 2; // info
	if (auto loglevel_from_env = hxcomm::get_loglevel_from_env("QUIGGELDY_LOGLEVEL");
	    loglevel_from_env) {
		loglevel = *loglevel_from_env;
	}
	logger_default_config(Logger::log4cxx_level_v2(loglevel));

	{
		auto args_for_quiggeldy = collect_args_for_quiggeldy(argv + 1);
		auto argv_offset = args_for_quiggeldy.size() + 1;
		[[maybe_unused]] hxcomm::EnsureLocalQuiggeldy quiggeldy{std::move(args_for_quiggeldy)};

		// perform execvp in fork because we need to destruct (i.e. kill
		// quiggeldy-binary) after the given command exits
		auto pid = fork();
		if (pid == 0) {
			execvp(argv[argv_offset], argv + argv_offset);
		} else {
			int status;
			waitpid(pid, &status, 0); // wait for the child to exit

			if (WIFEXITED(status)) {
				return WEXITSTATUS(status);
			} else if (WIFSIGNALED(status)) {
				auto signal = WTERMSIG(status);
				std::cerr << "Killed by: " << signal << " (" << strsignal(signal) << ")."
				          << std::endl;
				return signal;
			} else {
				std::cerr << "Wrapped process returned " << status << "." << std::endl;
				return status;
			}
		}
	}
}
