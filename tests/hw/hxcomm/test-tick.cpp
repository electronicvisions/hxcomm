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

TEST(TestConnection, TickRestrictedStream)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto const test = [](auto& connection) {
		std::vector<hxcomm::vx::UTMessageToFPGAVariant> messages;
		messages.push_back(UTMessageToFPGA<system::Loopback>(system::Loopback::tick));

		// halt message added by execute_messages
		auto [responses, connection_time_info] = hxcomm::execute_messages(connection, messages);

		EXPECT_EQ(responses.size(), 2);
		auto response_tick = responses.at(0);
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_tick),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::tick));
		auto response_halt = responses.at(1);
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_halt),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
	};

	auto connection = get_connection_from_env();
	std::visit(test, connection);
}

TEST(TestConnection, TickPerformance)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto const test = [](auto& connection, size_t const N) {
		// N ticks
		std::vector<hxcomm::vx::UTMessageToFPGAVariant> messages(
		    N, UTMessageToFPGA<system::Loopback>(system::Loopback::tick));

		// halt message added by execute_messages
		auto [responses, connection_time_info] = hxcomm::execute_messages(connection, messages);

		std::cout << connection_time_info << std::endl;

		EXPECT_EQ(responses.size(), N + 1);
		auto response_tick = responses.at(0);
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_tick),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::tick));
		auto response_halt = responses.at(N);
		EXPECT_EQ(
		    std::get<UTMessageFromFPGA<from_fpga_system::Loopback>>(response_halt),
		    UTMessageFromFPGA<from_fpga_system::Loopback>(from_fpga_system::Loopback::halt));
	};

	auto connection = get_connection_from_env();

	// TODO: cf. issue #3811
	std::vector<size_t> sizes({1, 1, 1, 10, 100, 1000, 10'000, 100'000, 1'000'000});
	for (size_t const n : sizes) {
		auto test_n = [&test, n](auto& connection) { test(connection, n); };
		std::visit(test_n, connection);
	}
}
