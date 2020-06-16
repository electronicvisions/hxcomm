#include "hxcomm/vx/connection_from_env.h"
#include <gtest/gtest.h>

TEST(TestConnection, Tick)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto const test = [](auto& connection) {
		hxcomm::Stream stream(connection);
		stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::tick));
		// Halt execution
		stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

		stream.commit();

		stream.run_until_halt();

		auto const responses = stream.receive_all();
		EXPECT_EQ(responses.size(), 2);
		auto response_tick = responses.at(0);
		auto response_halt = responses.at(1);
		EXPECT_TRUE(stream.receive_empty());
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_tick),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::tick));
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_halt),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
	};

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	if (std::holds_alternative<hxcomm::vx::ZeroMockConnection>(*connection)) {
		GTEST_SKIP() << "ZeroMockConnection does not support tick loopback.";
	}
	std::visit(test, *connection);
}
