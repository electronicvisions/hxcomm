#include <gtest/gtest.h>

#include "connection.h"
#include "hxcomm/test-helper.h"

using namespace hxcomm::test;

/**
 * Generate random JTAG data payload with a random number of sent bits.
 */
hxcomm::vx::instruction::to_fpga_jtag::Data::Payload random_data()
{
	using Data = hxcomm::vx::instruction::to_fpga_jtag::Data;
	// random number of data bits
	Data::Payload::NumBits num_bits(random_integer(
	    std::numeric_limits<Data::Payload::NumBits>::lowest(),
	    std::numeric_limits<Data::Payload::NumBits>::max()));
	// random data
	hate::bitset<Data::max_num_bits_payload> data_value =
	    random_bitset<Data::max_num_bits_payload>();
	// cap to num_bits bits
	data_value &=
	    ((~hate::bitset<Data::max_num_bits_payload>()) >>
	     (Data::max_num_bits_payload - static_cast<size_t>(num_bits)));
	// keep_response==true in order to get responses to check for equality
	return Data::Payload(true, num_bits, data_value);
}

/**
 * Write random payload JTAG data words in BYPASS mode and check equality to the write responses.
 */
TEST(TestConnection, JTAGLoopback)
{
	using namespace hxcomm;
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto connection = generate_test_connection();
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

	// Number of data words to write.
	constexpr size_t num = 10;

	std::vector<to_fpga_jtag::Data::Payload> payloads;
	for (size_t i = 0; i < num; ++i) {
		payloads.push_back(random_data());
	}

	// Select JTAG register that is guaranteed to have no effect
	stream.add(UTMessageToFPGA<to_fpga_jtag::Ins>(to_fpga_jtag::Ins::BYPASS));

	// add write messages
	for (auto payload : payloads) {
		stream.add(UTMessageToFPGA<to_fpga_jtag::Data>(payload));
	}

	// Halt execution
	stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10000)));
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	stream.run_until_halt();

	std::vector<UTMessageFromFPGAVariant> responses;

	TestConnection::receive_message_type message;
	while (stream.try_receive(message)) {
		responses.push_back(message);
	}

	// same amount of JTAG data responses as JTAG data instructions sent and a halt response.
	EXPECT_EQ(responses.size(), num + 1);

	for (size_t i = 0; i < num; ++i) {
		// shift by one because in between TDI and TDO there's one register in BYPASS mode
		auto response =
		    boost::get<UTMessageFromFPGA<jtag_from_hicann::Data>>(responses[i]).decode() >> 1;
		auto expected =
		    payloads[i].get_payload().reset(static_cast<size_t>(payloads[i].get_num_bits()) - 1);
		EXPECT_EQ(decltype(payloads[i].get_payload())(response), expected);
	}
}
