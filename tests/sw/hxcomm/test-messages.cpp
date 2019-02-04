#include <gtest/gtest.h>
#include "hxcomm/jtag.h"
#include "hxcomm/utmessage.h"

namespace {

class UTMessageFactoryTest : public ::testing::Test
{};

/// Tests generated JTAG instruction messages
TEST_F(UTMessageFactoryTest, MessageJTAGInstruction)
{
	using namespace hxcomm;
	EXPECT_EQ(UTMessageFactory::set_ir(JTAG::IDCODE), 0x0200000000000001);
}

} // namespace
