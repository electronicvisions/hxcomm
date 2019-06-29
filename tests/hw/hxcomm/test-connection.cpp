#include <gtest/gtest.h>

#include "connection.h"
#include "hxcomm/vx/connection_from_env.h"

using namespace hxcomm;
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

	auto stream = Stream(connection);

	EXPECT_TRUE(stream.receive_empty());
}

TEST(TestConnection, RunUntilHalt)
{
	auto connection = generate_test_connection();

	auto stream = Stream(connection);

	// Halt execution
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	EXPECT_NO_THROW(stream.run_until_halt());
}

TEST(TestConnection, Receive)
{
	auto connection = generate_test_connection();

	Stream stream(connection);

	// Halt execution
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	stream.run_until_halt();

	EXPECT_FALSE(stream.receive_empty());
	auto response = stream.receive();
	EXPECT_TRUE(stream.receive_empty());
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response),
	    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
}

TEST(TestConnection, TryReceive)
{
	auto connection = generate_test_connection();

	Stream stream(connection);

	// Halt execution
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	stream.run_until_halt();

	EXPECT_FALSE(stream.receive_empty());
	UTMessageFromFPGAVariant response;
	EXPECT_TRUE(stream.try_receive(response));
	EXPECT_TRUE(stream.receive_empty());
	EXPECT_EQ(
	    boost::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response),
	    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
}

TEST(TestConnection, FromEnv)
{
	// Just ensure that this compiles for now
	[[maybe_unused]] auto connection = hxcomm::vx::get_connection_from_env();
}
