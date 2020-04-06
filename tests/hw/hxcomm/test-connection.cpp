#include <gtest/gtest.h>

#include "connection.h"

using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

TEST(TestConnection, Moveable)
{
	auto connection = generate_test_connection();

	auto moved_to_connection = std::move(connection);
}

TEST(TestConnection, ReceiveEmpty)
{
	auto connection = generate_test_connection();

	EXPECT_TRUE(connection.receive_empty());
}

TEST(TestConnection, RunUntilHalt)
{
	auto connection = generate_test_connection();

	// Halt execution
	connection.add(UTMessageToFPGA<system::Halt>());

	connection.commit();

	EXPECT_NO_THROW(connection.run_until_halt());
}

TEST(TestConnection, Receive)
{
	auto connection = generate_test_connection();

	// Halt execution
	connection.add(UTMessageToFPGA<system::Halt>());

	connection.commit();

	connection.run_until_halt();

	EXPECT_FALSE(connection.receive_empty());
	auto response = connection.receive();
	EXPECT_TRUE(connection.receive_empty());
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Halt>>(response),
	    UTMessageFromFPGA<from_fpga_system::Halt>());
}

TEST(TestConnection, TryReceive)
{
	auto connection = generate_test_connection();

	// Halt execution
	connection.add(UTMessageToFPGA<system::Halt>());

	connection.commit();

	connection.run_until_halt();

	EXPECT_FALSE(connection.receive_empty());
	UTMessageFromFPGAVariant response;
	EXPECT_TRUE(connection.try_receive(response));
	EXPECT_TRUE(connection.receive_empty());
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Halt>>(response),
	    UTMessageFromFPGA<from_fpga_system::Halt>());
}
