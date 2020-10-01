
#include "hxcomm/common/ensure_local_quiggeldy.h"
#include "hxcomm/common/quiggeldy_utility.h"

#include "logging_ctrl.h"

#include <RCF/RCF.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <tuple>
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
	std::cerr << "Usage: " << argv[0] << " EXECUTABLE [ARGS...]\n\n"
	          << "    Ensure a local quiggeldy is running (and set in env) prior to running\n"
	             "    the given executable with the provided arguments.\n\n"
	             "    By default quiggeldy will exit after idling for 10 seconds.\n"
	             "    You can specify QUIGGELDY_TIMEOUT to overwrite the timeout.\n\n"
	          << std::endl;
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
		[[maybe_unused]] hxcomm::EnsureLocalQuiggeldy quiggeldy{};

		// perform execvp in fork because we need to destruct (i.e. kill
		// quiggeldy-binary) after the given command exits
		auto pid = fork();
		if (pid < 0) {
			perror("Could not fork away.");
			return EXIT_FAILURE;
		} else if (pid == 0) {
			execvp(argv[1], argv + 1);
		} else {
			int status = 0;
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
