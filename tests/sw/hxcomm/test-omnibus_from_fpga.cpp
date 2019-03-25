#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/omnibus_from_fpga.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction::omnibus_from_fpga;

TEST(omnibus_from_fpga_data, EncodeDecode)
{
	typename data::payload_type payload(random_bitset<data::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
