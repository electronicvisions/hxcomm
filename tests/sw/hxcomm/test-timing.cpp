#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/timing.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(Setup, EncodeDecode)
{
	typename timing::Setup::Payload payload;

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}


TEST(WaitUntil, EncodeDecode)
{
	typename timing::WaitUntil::Payload payload(draw_non_default_value<uint32_t>(0));

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}


TEST(SystimeInit, EncodeDecode)
{
	typename timing::SystimeInit::Payload payload(random_bitset<timing::SystimeInit::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
