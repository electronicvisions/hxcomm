#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/omnibus_to_fpga.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction::omnibus_to_fpga;

TEST(omnibus_to_fpga_address, EncodeDecode)
{
	auto addr = draw_non_default_value<uint32_t>(0);
	bool read = static_cast<bool>(random_integer(0, 1));
	auto byte_enables = random_bitset<4>();

	typename address::payload_type payload(addr, read, byte_enables);

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(omnibus_to_fpga_data, EncodeDecode)
{
	auto data_value = random_bitset<data::size>();
	typename data::payload_type payload(data_value);

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}