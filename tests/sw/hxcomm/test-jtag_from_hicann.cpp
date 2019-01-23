#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/jtag_from_hicann.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(jtag_from_hicann_data, EncodeDecode)
{
	typename jtag_from_hicann::data::payload_type payload(
	    random_bitset<jtag_from_hicann::data::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
