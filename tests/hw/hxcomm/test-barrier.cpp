#include <gtest/gtest.h>

#include "connection.h"

TEST(TestConnection, Barrier)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto connection = generate_test_connection();
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
}
