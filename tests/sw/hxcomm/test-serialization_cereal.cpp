#include <gtest/gtest.h>

#include "hxcomm/vx/utmessage.h"

using namespace hxcomm::vx;

template <class T>
class CommonSerializationTests : public ::testing::Test
{};

typedef ::testing::Types<
    ut_message_to_fpga<instruction::to_fpga_jtag::init>,
    ut_message_to_fpga<instruction::to_fpga_jtag::scaler>,
    ut_message_to_fpga<instruction::to_fpga_jtag::ins>,
    ut_message_to_fpga<instruction::to_fpga_jtag::data>,
    ut_message_to_fpga<instruction::timing::setup>,
    ut_message_to_fpga<instruction::timing::wait_until>,
    ut_message_to_fpga<instruction::system::reset>,
    ut_message_to_fpga<instruction::system::halt>,
    ut_message_to_fpga<instruction::omnibus_to_fpga::address>,
    ut_message_to_fpga<instruction::omnibus_to_fpga::data>,
    ut_message_from_fpga<instruction::jtag_from_hicann::data>,
    ut_message_from_fpga<instruction::omnibus_from_fpga::data>,
    ut_message_from_fpga<instruction::from_fpga_system::halt> >
    SerializableTypes;

TYPED_TEST_CASE(CommonSerializationTests, SerializableTypes);

TYPED_TEST(CommonSerializationTests, IsDefaultConstructible)
{
	TypeParam obj;
	static_cast<void>(&obj);
}

TYPED_TEST(CommonSerializationTests, IsAssignable)
{
	TypeParam obj1, obj2;
	obj1 = obj2;
}

TYPED_TEST(CommonSerializationTests, HasSerialization)
{
	TypeParam obj1, obj2;

	std::ostringstream ostream;
	{
		cereal::JSONOutputArchive oa(ostream);
		oa(obj1);
	}

	std::istringstream istream(ostream.str());
	{
		cereal::JSONInputArchive ia(istream);
		ia(obj2);
	}
	// This does only test that Serialization does not insert wrong values
	// but does not check coverage since both instances are default constructed.
	// Coverage check is done in each container's test file.
	ASSERT_EQ(obj2, obj1);
}
