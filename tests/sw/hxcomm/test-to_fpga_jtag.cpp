#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/to_fpga_jtag.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(init, EncodeDecode)
{
	typename to_fpga_jtag::init::payload_type payload;

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(scaler, EncodeDecode)
{
	typename to_fpga_jtag::scaler::payload_type payload(draw_non_default_value(0));

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(ins, EncodeDecode)
{
	auto payload = draw_ranged_non_default_value<to_fpga_jtag::ins::payload_type>(0);

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(to_fpga_jtag_data, EncodeDecode)
{
	EXPECT_EQ(to_fpga_jtag::data::payload_type::NumBits::min, 3);
	EXPECT_EQ(to_fpga_jtag::data::payload_type::NumBits::max, 33);
	auto const num_bits =
	    draw_ranged_non_default_value<to_fpga_jtag::data::payload_type::NumBits>(33);
	auto const jtag_payload = random_bitset<33>();
	bool const keep_response = true;

	auto payload = to_fpga_jtag::data::payload_type(keep_response, num_bits, jtag_payload);

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
