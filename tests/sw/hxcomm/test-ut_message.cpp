#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hxcomm/vx/utmessage.h"

using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

template <class T>
class CommonUTMessageTests : public ::testing::Test
{};

template <typename ToTL, typename FromTL>
struct to_testing_types;

template <typename... ToIs, typename... FromIs>
struct to_testing_types<hate::type_list<ToIs...>, hate::type_list<FromIs...>>
{
	typedef ::testing::Types<ut_message_to_fpga<ToIs>..., ut_message_from_fpga<FromIs>...> type;
};

typedef
    typename to_testing_types<instruction::to_fpga_dictionary, instruction::from_fpga_dictionary>::
        type UTMessageTypes;

TYPED_TEST_CASE(CommonUTMessageTests, UTMessageTypes);

TYPED_TEST(CommonUTMessageTests, General)
{
	typedef TypeParam ut_message_t;

	ut_message_t config;

	ASSERT_EQ(
	    config.get_header(),
	    typename ut_message_t::header_type(hate::index_type_list_by_type<
	                                       typename ut_message_t::instruction_type,
	                                       typename ut_message_t::dictionary_type>::value));
	ASSERT_EQ(config.get_payload(), typename ut_message_t::payload_type());

	if constexpr (ut_message_t::payload_type::size) {
		typename ut_message_t::payload_type payload;
		payload.set(0, true);
		config.set_payload(payload);
		ASSERT_EQ(config.get_payload(), payload);

		ut_message_t config_eq = config;
		ut_message_t config_ne = config;
		config_ne.set_payload(~config_ne.get_payload());

		ASSERT_EQ(config, config_eq);
		ASSERT_FALSE(config == config_ne);

		ASSERT_NE(config, config_ne);
		ASSERT_FALSE(config != config_eq);
	}
}

TYPED_TEST(CommonUTMessageTests, CerealizeCoverage)
{
	TypeParam obj1, obj2;

	obj1.set_payload(~obj1.get_payload());

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
	ASSERT_EQ(obj1, obj2);
}
