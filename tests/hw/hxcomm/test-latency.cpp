#include "hate/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include <algorithm>
#include <numeric>
#include <gtest/gtest.h>

using namespace hxcomm;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

TEST(TestConnection, Latency)
{
	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}

	if (std::holds_alternative<hxcomm::vx::SimConnection>(*connection)) {
		GTEST_SKIP() << "Latency test not valid in simulation.";
	}

	auto const test = [](auto& connection) {
		auto stream = Stream(connection);

		constexpr size_t num = 10000;

		std::vector<size_t> durations_us;
		for (size_t i = 0; i < num; ++i) {
			stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));
			hate::Timer timer;
			// measure from starting to send until receiving and decoding the halt response
			stream.commit();
			stream.run_until_halt();
			durations_us.push_back(timer.get_us());
		}
		auto const minimum = *std::min_element(durations_us.begin(), durations_us.end());
		auto const mean =
		    std::accumulate(durations_us.begin(), durations_us.end(), static_cast<size_t>(0)) / num;

		std::nth_element(
		    durations_us.begin(), durations_us.begin() + durations_us.size() / 2,
		    durations_us.end());
		auto const median = durations_us.at(num / 2);

		auto logger = log4cxx::Logger::getLogger("hxcomm.backendtest.Throughput");
		HXCOMM_LOG_INFO(logger, "Minimal latency: " << minimum << " us");
		HXCOMM_LOG_INFO(logger, "Average latency: " << mean << " us");
		HXCOMM_LOG_INFO(logger, "Median latency: " << median << " us");

		EXPECT_LE(minimum, 120); // typically below 60us, 100% error margin
		EXPECT_LE(mean, 200);
		EXPECT_LE(median, 150);
	};

	std::visit(test, *connection);
}
