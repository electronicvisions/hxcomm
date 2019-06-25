#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/system.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(Reset, EncodeDecode)
{
	typename system::Reset::Payload payload(random_bitset<1>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
