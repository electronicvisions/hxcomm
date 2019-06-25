#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/jtag_from_hicann.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(jtag_from_hicann_Data, EncodeDecode)
{
	typename jtag_from_hicann::Data::Payload payload(random_bitset<jtag_from_hicann::Data::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
