#include <gtest/gtest.h>

#include "connection.h"

TEST(TestConnection, ReadoutJtagID)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;
	using Data = to_fpga_jtag::Data;

	auto connection = generate_test_connection();

	// Reset sequence
	connection.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(true)));
	connection.add(UTMessageToFPGA<timing::Setup>());
	connection.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10)));
	connection.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(false)));
	connection.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(100)));

	// JTAG init
	connection.add(UTMessageToFPGA<to_fpga_jtag::Scaler>(3));
	connection.add(UTMessageToFPGA<to_fpga_jtag::Init>());

	// Read ID (JTAG instruction register is by specification IDCODE after init)
	connection.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0)));

	// Halt execution
	connection.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10000)));
	connection.add(UTMessageToFPGA<system::Halt>());

	connection.commit();

	connection.run_until_halt();

	std::vector<UTMessageFromFPGAVariant> responses;
	while (true) {
		try {
			responses.push_back(connection.receive());
		} catch (std::runtime_error& ignored) {
			break;
		}
	}
	EXPECT_EQ(responses.size(), 2);
	EXPECT_EQ(
	    static_cast<uint32_t>(
	        boost::get<UTMessageFromFPGA<jtag_from_hicann::Data>>(responses.front()).decode()),
	    0x48580AF);
}
