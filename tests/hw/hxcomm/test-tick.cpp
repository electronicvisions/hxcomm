#include <gtest/gtest.h>

#include "connection.h"

TEST(TestConnection, Tick)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto connection = generate_test_connection();
	hxcomm::Stream stream(connection);

	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::tick));
	// Halt execution
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	stream.run_until_halt();

	auto response_tick = stream.receive();
	auto response_halt = stream.receive();
	EXPECT_TRUE(stream.receive_empty());
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_tick),
	    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::tick));
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_halt),
	    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
}