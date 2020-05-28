#include <gtest/gtest.h>

#include "hate/math.h"
#include "hate/timer.h"
#include "hxcomm/vx/connection_from_env.h"

template <typename Connection>
double test_throughput(size_t const num, Connection& connection)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto to_mega_rate = [](size_t count, auto dur_us) -> double {
		return static_cast<double>(count) / static_cast<double>(dur_us);
	};

	std::vector<UTMessageToFPGAVariant> instructions;

	for (size_t i = 0; i < num; ++i) {
		// Use timer setup on FPGA because it is processed in one FPGA cycle.
		// This is not representative for a random message stream.
		instructions.push_back(UTMessageToFPGA<timing::Setup>());
	}
	instructions.push_back(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	size_t byte_count = 0;

	// calculate number of bytes of encoded messages
	for (auto msg : instructions) {
		std::visit([&byte_count](auto&& m) { byte_count += m.word_width / CHAR_BIT; }, msg);
	}

	hate::Timer timer;

	connection.add(instructions.begin(), instructions.end());
	connection.commit();
	connection.run_until_halt();
	auto response = connection.receive_all();

	auto us = timer.get_us();
	auto const mega_rate = to_mega_rate(byte_count, us);

	auto logger = log4cxx::Logger::getLogger("hxcomm.backendtest.Throughput");
	HXCOMM_LOG_INFO(logger, "To FPGA rate for " << byte_count << " B: " << mega_rate << " MB/s");
	return mega_rate;
}

TEST(TestConnection, Throughput)
{
	auto const test = [](auto& connection) {
		hxcomm::Stream stream(connection);
		constexpr size_t max_pow = 29;
		[[maybe_unused]] double mega_rate = 0;
		for (size_t pow = 0; pow < max_pow; ++pow) {
			mega_rate = test_throughput(hate::math::pow(2, pow), stream);
			EXPECT_TRUE(stream.receive_empty());
		}
		// Issue #3978
		// EXPECT_GT(
		//     mega_rate, 125. * 0.8); // reach minimally 80 % of 1GBit in the limit of large
		//     programs
	};

	auto connection = hxcomm::vx::get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	if (std::holds_alternative<hxcomm::vx::SimConnection>(*connection)) {
		GTEST_SKIP() << "Throughput measurement only works for hardware.";
	}
	std::visit(test, *connection);
}
