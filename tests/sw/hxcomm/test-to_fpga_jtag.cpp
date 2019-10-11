#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/to_fpga_jtag.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(Init, EncodeDecode)
{
	typename to_fpga_jtag::Init::Payload payload;

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(Scaler, EncodeDecode)
{
	typename to_fpga_jtag::Scaler::Payload payload(draw_non_default_value(0));

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(Ins, EncodeDecode)
{
	auto payload = to_fpga_jtag::Ins::Payload(random_bitset<to_fpga_jtag::Ins::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(to_fpga_jtag_Data, EncodeDecode)
{
	EXPECT_EQ(to_fpga_jtag::Data::Payload::NumBits::min, 3);
	EXPECT_EQ(to_fpga_jtag::Data::Payload::NumBits::max, 33);
	auto const num_bits = draw_ranged_non_default_value<to_fpga_jtag::Data::Payload::NumBits>(33);
	auto const jtag_payload = random_bitset<33>();
	bool const keep_response = true;

	auto payload = to_fpga_jtag::Data::Payload(keep_response, num_bits, jtag_payload);

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
