#include "hxcomm/vx/connection_from_env.h"
#include <gtest/gtest.h>

TEST(TestConnection, ReadoutJtagID)
{
	using namespace hxcomm;
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;
	using Data = to_fpga_jtag::Data;

	std::vector<UTMessageFromFPGAVariant> responses;
	auto const test = [&responses](auto& connection) {
		auto stream = Stream(connection);

		// Reset sequence
		stream.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(true)));
		stream.add(UTMessageToFPGA<timing::Setup>());
		stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10)));
		stream.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(false)));
		stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(100)));

		// JTAG init
		stream.add(UTMessageToFPGA<to_fpga_jtag::Scaler>(3));
		stream.add(UTMessageToFPGA<to_fpga_jtag::Init>());

		// Read ID (JTAG instruction register is by specification IDCODE after init)
		stream.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0)));

		// Halt execution
		stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10000)));
		stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

		stream.commit();

		stream.run_until_halt();

		responses = stream.receive_all();
	};

	auto connection = get_connection_full_stream_interface_from_env();
	if (!connection) {
		GTEST_SKIP();
	}
	if (std::holds_alternative<hxcomm::vx::ZeroMockConnection>(*connection)) {
		GTEST_SKIP() << "ZeroMockConnection does not support JTAG loopback.";
	}
	std::visit(test, *connection);

	EXPECT_EQ(responses.size(), 2);
	auto jtag_id = static_cast<uint32_t>(
	    std::get<UTMessageFromFPGA<jtag_from_hicann::Data>>(responses.front()).decode());
	EXPECT_TRUE(
	    (jtag_id == 0x048580AF) || (jtag_id == 0x248580AF) || (jtag_id == 0x348580AF) ||
	    (jtag_id == 0x448580AF));
}
