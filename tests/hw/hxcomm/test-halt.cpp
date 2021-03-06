#include <gtest/gtest.h>

#include "hxcomm/vx/connection_from_env.h"

TEST(TestConnection, Halt)
{
	using namespace hxcomm;
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto const test = [](auto& connection) {
		auto stream = Stream(connection);

		// Halt execution
		stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

		stream.commit();

		stream.run_until_halt();

		auto const responses = stream.receive_all();
		EXPECT_EQ(responses.size(), 1);
		auto const response = responses.at(0);
		EXPECT_TRUE(stream.receive_empty());
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
	};

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	std::visit(test, *connection);
}
