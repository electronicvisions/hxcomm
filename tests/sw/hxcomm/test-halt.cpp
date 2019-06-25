#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/from_fpga_system.h"
#include "hxcomm/vx/instruction/system.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(Halt, EncodeDecode)
{
	typename system::Halt::Payload payload;

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}


TEST(HaltResponse, EncodeDecode)
{
	typename from_fpga_system::Halt::Payload payload;

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
