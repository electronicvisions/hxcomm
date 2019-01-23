#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/timing.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(setup, EncodeDecode)
{
	typename timing::setup::payload_type payload;

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}


TEST(wait_until, EncodeDecode)
{
	typename timing::wait_until::payload_type payload(draw_non_default_value<uint32_t>(0));

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
