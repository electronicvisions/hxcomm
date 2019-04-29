#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/timing_from_fpga.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(Systime, EncodeDecode)
{
	typename timing_from_fpga::Systime::Payload payload(
	    random_bitset<timing_from_fpga::Systime::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(Sysdelta, EncodeDecode)
{
	typename timing_from_fpga::Sysdelta::Payload payload(
	    random_bitset<timing_from_fpga::Sysdelta::size>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
