#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/omnibus_from_fpga.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction::omnibus_from_fpga;

TEST(omnibus_from_fpga_Data, EncodeDecode)
{
	typename Data::Payload payload(random_bitset<Data::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
