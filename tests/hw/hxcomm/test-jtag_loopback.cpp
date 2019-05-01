#include <gtest/gtest.h>

#include "connection.h"
#include "test-helper.h"

/**
 * Generate random JTAG data payload with a random number of sent bits.
 */
hxcomm::vx::instruction::to_fpga_jtag::data::payload_type random_data()
{
	using data = hxcomm::vx::instruction::to_fpga_jtag::data;
	// random number of data bits
	data::payload_type::NumBits num_bits(
	    random_integer(data::payload_type::NumBits::min, data::payload_type::NumBits::max));
	// random data
	hate::bitset<data::max_num_bits_payload> data_value =
	    random_bitset<data::max_num_bits_payload>();
	// cap to num_bits bits
	data_value &=
	    ((~hate::bitset<data::max_num_bits_payload>()) >> (data::max_num_bits_payload - num_bits));
	// keep_response==true in order to get responses to check for equality
	return data::payload_type(true, num_bits, data_value);
}

/**
 * Write random payload JTAG data words in BYPASS mode and check equality to the write responses.
 */
TEST(TestConnection, JTAGLoopback)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	auto connection = generate_test_connection();

	// Reset sequence
	connection.add(ut_message_to_fpga<system::reset>(system::reset::payload_type(true)));
	connection.add(ut_message_to_fpga<timing::setup>());
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(10)));
	connection.add(ut_message_to_fpga<system::reset>(system::reset::payload_type(false)));
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(100)));

	// JTAG init
	connection.add(ut_message_to_fpga<to_fpga_jtag::scaler>(3));
	connection.add(ut_message_to_fpga<to_fpga_jtag::init>());

	// Number of data words to write.
	constexpr size_t num = 10;

	std::vector<to_fpga_jtag::data::payload_type> payloads;
	for (size_t i = 0; i < num; ++i) {
		payloads.push_back(random_data());
	}

	// Select JTAG register that is guaranteed to have no effect
	connection.add(ut_message_to_fpga<to_fpga_jtag::ins>(to_fpga_jtag::ins::BYPASS));

	// add write messages
	for (auto payload : payloads) {
		connection.add(ut_message_to_fpga<to_fpga_jtag::data>(payload));
	}

	// Halt execution
	connection.add(ut_message_to_fpga<timing::wait_until>(timing::wait_until::payload_type(10000)));
	connection.add(ut_message_to_fpga<system::halt>());

	connection.commit();

	connection.run_until_halt();

	std::vector<ut_message_from_fpga_variant> responses;

	TestConnection::receive_message_type message;
	while (connection.try_receive(message)) {
		responses.push_back(message);
	}

	// same amount of JTAG data responses as JTAG data instructions sent and a halt response.
	EXPECT_EQ(responses.size(), num + 1);

	for (size_t i = 0; i < num; ++i) {
		// shift by one because in between TDI and TDO there's one register in BYPASS mode
		auto response =
		    boost::get<ut_message_from_fpga<jtag_from_hicann::data>>(responses[i]).decode() >> 1;
		auto expected = payloads[i].get_payload().reset(payloads[i].get_num_bits() - 1);
		EXPECT_EQ(decltype(payloads[i].get_payload())(response), expected);
	}
}
