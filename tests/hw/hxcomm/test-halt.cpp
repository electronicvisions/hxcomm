#include <gtest/gtest.h>

#include "connection.h"

TEST(TestConnection, Halt)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto connection = generate_test_connection();

	// Halt execution
	connection.add(UTMessageToFPGA<system::Halt>());

	connection.commit();

	connection.run_until_halt();

	auto response = connection.receive();
	EXPECT_TRUE(connection.receive_empty());
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Halt>>(response),
	    UTMessageFromFPGA<from_fpga_system::Halt>());
}
