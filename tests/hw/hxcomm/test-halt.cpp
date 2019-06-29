#include <gtest/gtest.h>

#include "connection.h"

TEST(TestConnection, Halt)
{
	using namespace hxcomm;
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto connection = generate_test_connection();
	auto stream = Stream(connection);

	// Halt execution
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	stream.run_until_halt();

	auto response = stream.receive();
	EXPECT_TRUE(stream.receive_empty());
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response),
	    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
}
