#include "gtest/gtest.h"
#include "hxcomm/vx/utmessage.h"

TEST(Dictionary, ToFPGA)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	using type_4 = hate::index_type_list_by_integer<4, ToFPGADictionary>::type;
	EXPECT_TRUE((std::is_same<type_4, timing::Setup>::value));

	constexpr size_t index_8 =
	    hate::index_type_list_by_type<system::Reset, ToFPGADictionary>::value;
	EXPECT_EQ(index_8, 8);

	EXPECT_EQ(type_4::size, 0);
	EXPECT_EQ((hate::index_type_list_by_integer<index_8, ToFPGADictionary>::type::size), 1);
	EXPECT_EQ((jtag_from_hicann::Data::size), 33);
}

TEST(Dictionary, FromFPGA)
{
	using namespace hxcomm::vx;
	using namespace hxcomm::vx::instruction;

	using type_0 = hate::index_type_list_by_integer<0, FromFPGADictionary>::type;
	EXPECT_TRUE((std::is_same<type_0, jtag_from_hicann::Data>::value));

	EXPECT_EQ(type_0::size, 33);
}
