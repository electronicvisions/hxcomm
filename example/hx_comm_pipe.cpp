#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdlib.h>
#include <thread>

#include <boost/program_options.hpp>

#include "flange/simulator_client.h"

#include "hx_comm_pipe_async_io_helper.h"

void signal_handler(int /*signum*/)
{
	exit(EXIT_FAILURE);
}

namespace bpo = boost::program_options;

int main(int argc, char* argv[])
{
	signal(SIGTERM, &signal_handler);
	signal(SIGINT, &signal_handler);

	// parse arguments
	std::string str_ip;
	int port;
	unsigned int rcf_response_timeout;
	size_t wait;
	bpo::options_description desc("Options");
	// clang-format off
	desc.add_options()("help", "produce help message")
	    ("ip", bpo::value<std::string>(&str_ip)->default_value("127.0.0.1"), "Simulation server IP")
	    ("port", bpo::value<int>(&port)->default_value(50001), "Simulation server port")
	    ("rcf-response-timeout", bpo::value<unsigned int>(&rcf_response_timeout)->default_value(60 * 1000),
	    "RCF communication timeout [ms]")
	    ("simulation-time", bpo::value<size_t>(&wait)->default_value(1000),
	    "Simulation time [cycles]");
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
	bpo::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	// Instantiate a Simulation client.
	std::unique_ptr<flange::SimulatorClient> sim =
	    std::make_unique<flange::SimulatorClient>(str_ip, port);
	sim->set_remote_timeout(rcf_response_timeout);

	AsyncStreamReader reader{&std::cin};

	bool first = true;
	while (sim->get_current_time() < wait) {
		if (reader.is_ready()) {
			if (first) {
				sim->set_runnable(true);
				first = false;
			}
			auto input_line = reader.get_line();
			uint64_t message = strtoull(input_line.c_str(), NULL, 16);
			sim->send(message);
		}

		while (sim->receive_data_available()) {
			auto message = sim->receive();
			std::cout << std::setfill('0') << std::setw(16) << std::hex << message << std::endl;
		}
	}
}
