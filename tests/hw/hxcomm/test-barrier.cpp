#include <gtest/gtest.h>

#include "hxcomm/vx/connection_from_env.h"

TEST(TestConnection, Barrier)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto const test = [](auto& connection) {
		auto stream = hxcomm::Stream(connection);

		auto const run = [&stream](auto const message) {
			stream.add(message);
			stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));
			stream.commit();
			stream.run_until_halt();
		};

		// block until nothing
		run(UTMessageToFPGA<timing::Barrier>());

		// block until Omnibus is idle
		run(UTMessageToFPGA<timing::Barrier>(timing::Barrier::omnibus));

		// block until JTAG is idle
		run(UTMessageToFPGA<timing::Barrier>(timing::Barrier::jtag));

		// block until Omnibus and JTAG are idle
		run(UTMessageToFPGA<timing::Barrier>(timing::Barrier::omnibus | timing::Barrier::jtag));
	};

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	std::visit(test, *connection);
}
