#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/from_fpga_system.h"
#include "hxcomm/vx/instruction/system.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

TEST(halt, EncodeDecode)
{
	typename system::halt::payload_type payload;

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}


TEST(halt_response, EncodeDecode)
{
	typename from_fpga_system::halt::payload_type payload(
	    from_fpga_system::halt::payload_type::ToFpgaCount(draw_non_default_value<uint32_t>(0)),
	    from_fpga_system::halt::payload_type::FromFpgaCount(draw_non_default_value<uint32_t>(0)));

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}
