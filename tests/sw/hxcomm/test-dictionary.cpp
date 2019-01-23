#include "gtest/gtest.h"
#include "hxcomm/vx/utmessage.h"

TEST(Dictionary, ToFPGA)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	using type_4 = hate::index_type_list_by_integer<4, to_fpga_dictionary>::type;
	EXPECT_TRUE((std::is_same<type_4, timing::setup>::value));

	constexpr size_t index_6 =
	    hate::index_type_list_by_type<system::reset, to_fpga_dictionary>::value;
	EXPECT_EQ(index_6, 6);

	EXPECT_EQ(type_4::size, 0);
	EXPECT_EQ((hate::index_type_list_by_integer<index_6, to_fpga_dictionary>::type::size), 1);
	EXPECT_EQ((jtag_from_hicann::data::size), 33);
}

TEST(Dictionary, FromFPGA)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	using type_0 = hate::index_type_list_by_integer<0, from_fpga_dictionary>::type;
	EXPECT_TRUE((std::is_same<type_0, jtag_from_hicann::data>::value));

	EXPECT_EQ(type_0::size, 33);
}
