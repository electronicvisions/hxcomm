#include "hxcomm/vx/connection_from_env.h"
#include <gtest/gtest.h>

using namespace hxcomm;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

TEST(TestConnection, Moveable)
{
	auto connection = get_connection_from_env();

	auto moved_to_connection = std::move(connection);
}

TEST(TestConnection, MoveAssignable)
{
	auto const test = [](auto& connection) {
		auto stream = Stream(connection);

		// Halt execution
		stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

		stream.commit();

		EXPECT_NO_THROW(stream.run_until_halt());
	};

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	// move constructor
	auto moved_to_connection = std::move(connection);
	// move assignment operator
	connection = std::move(moved_to_connection);

	std::visit(test, *connection);
}

TEST(TestConnection, ReceiveEmpty)
{
	auto const test = [](auto& connection) {
		auto stream = Stream(connection);

		EXPECT_TRUE(stream.receive_empty());
	};

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	std::visit(test, *connection);
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

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	std::visit(test, *connection);
}

TEST(TestConnection, ReceiveAll)
{
	auto const test = [](auto& connection) {
		Stream stream(connection);

		// Halt execution
		stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

		stream.commit();

		stream.run_until_halt();

		EXPECT_FALSE(stream.receive_empty());
		auto responses = stream.receive_all();
		EXPECT_EQ(responses.size(), 1);
		EXPECT_TRUE(stream.receive_empty());
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(responses.at(0)),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
	};

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	std::visit(test, *connection);
}

TEST(TestConnection, FromEnv)
{
	// Just ensure that this compiles for now
	[[maybe_unused]] auto connection = hxcomm::vx::get_connection_from_env();
}

TEST(TestConnection, Registry)
{
	{
		auto connection = get_connection_from_env();
		if (std::holds_alternative<hxcomm::vx::ARQConnection>(connection) ||
		    std::holds_alternative<hxcomm::vx::SimConnection>(connection)) {
			EXPECT_THROW(get_connection_from_env(), std::runtime_error);
		}
	}
	EXPECT_NO_THROW(get_connection_from_env());
}
