#include <gtest/gtest.h>

#include "hxcomm/vx/connection_from_env.h"

using namespace hxcomm;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

TEST(TestConnection, Moveable)
{
	auto connection = get_connection_from_env();

	auto moved_to_connection = std::move(connection);
}

TEST(TestConnection, ReceiveEmpty)
{
	auto const test = [](auto& connection) {
		auto stream = Stream(connection);

		EXPECT_TRUE(stream.receive_empty());
	};

	auto connection = get_connection_from_env();
	std::visit(test, connection);
}

TEST(TestConnection, RunUntilHalt)
{
	auto const test = [](auto& connection) {
		auto stream = Stream(connection);

		// Halt execution
		stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

		stream.commit();

		EXPECT_NO_THROW(stream.run_until_halt());
	};

	auto connection = get_connection_from_env();
	std::visit(test, connection);
}

TEST(TestConnection, Receive)
{
	auto const test = [](auto& connection) {
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
	};

	auto connection = get_connection_from_env();
	std::visit(test, connection);
}

TEST(TestConnection, TryReceive)
{
	auto const test = [](auto& connection) {
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
	};

	auto connection = get_connection_from_env();
	std::visit(test, connection);
}

TEST(TestConnection, FromEnv)
{
	// Just ensure that this compiles for now
	[[maybe_unused]] auto connection = hxcomm::vx::get_connection_from_env();
}
