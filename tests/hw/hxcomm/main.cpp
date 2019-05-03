#include <string>

#include <boost/program_options.hpp>
#include <gtest/gtest.h>

#include "connection.h"
// logger include directory structure omits prefix
#include "logging_ctrl.h"

#ifndef HXCOMM_TEST_ARQ_CONNECTION
std::string simulation_ip;
unsigned int simulation_port;
#endif

TestConnection generate_test_connection()
{
#ifdef HXCOMM_TEST_ARQ_CONNECTION
	return TestConnection();
#else
	return TestConnection(simulation_ip, simulation_port);
#endif
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);

	std::string loglevel;
	namespace bpo = boost::program_options;
	bpo::options_description desc("Options");
	// clang-format off
#ifndef HXCOMM_TEST_ARQ_CONNECTION
	desc.add_options()("simulation_ip", bpo::value<std::string>(&simulation_ip)->default_value("127.0.0.1"));
	desc.add_options()("simulation_port", bpo::value<unsigned int>(&simulation_port)->required());
#endif
	desc.add_options()("loglevel", bpo::value<std::string>(&loglevel)->default_value("trace"));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(
	    bpo::basic_command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
	bpo::notify(vm);

	if (loglevel == "trace") {
		logger_default_config(log4cxx::Level::getTrace());
	} else if (loglevel == "debug") {
		logger_default_config(log4cxx::Level::getDebug());
	} else if (loglevel == "info") {
		logger_default_config(log4cxx::Level::getInfo());
	} else if (loglevel == "warning") {
		logger_default_config(log4cxx::Level::getWarn());
	} else if (loglevel == "error") {
		logger_default_config(log4cxx::Level::getError());
	} else if (loglevel == "fatal") {
		logger_default_config(log4cxx::Level::getFatal());
	} else {
		std::cout << "loglevel option has to be one of {trace,debug,info,warning,error,fatal"
		          << std::endl;
		exit(EXIT_FAILURE);
	}

	return RUN_ALL_TESTS();
}
