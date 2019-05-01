#include <string>

#include <boost/program_options.hpp>
#include <gtest/gtest.h>

#include "connection.h"

#ifdef HXCOMM_TEST_ARQ_CONNECTION
std::string fpga_ip;
#else
std::string simulation_ip;
unsigned int simulation_port;
#endif

TestConnection generate_test_connection()
{
#ifdef HXCOMM_TEST_ARQ_CONNECTION
	return TestConnection(fpga_ip);
#else
	return TestConnection(simulation_ip, simulation_port);
#endif
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);

#ifdef HXCOMM_TEST_ARQ_CONNECTION
	namespace bpo = boost::program_options;
	bpo::options_description desc("Options");
	// clang-format off
	desc.add_options()("fpga_ip", bpo::value<std::string>(&fpga_ip)->required());
	// clang-format on

	bpo::variables_map vm;
	bpo::store(
	    bpo::basic_command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
	bpo::notify(vm);
#else
	namespace bpo = boost::program_options;
	bpo::options_description desc("Options");
	// clang-format off
	desc.add_options()("simulation_ip", bpo::value<std::string>(&simulation_ip)->default_value("127.0.0.1"));
	desc.add_options()("simulation_port", bpo::value<unsigned int>(&simulation_port)->required());
	// clang-format on

	bpo::variables_map vm;
	bpo::store(
	    bpo::basic_command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
	bpo::notify(vm);
#endif

	return RUN_ALL_TESTS();
}
