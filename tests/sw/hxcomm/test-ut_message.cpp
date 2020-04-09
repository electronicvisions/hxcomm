#include <boost/type_index.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hxcomm/common/cerealization_utmessage.h"
#include "hxcomm/vx/payload_random.h"
#include "hxcomm/vx/utmessage.h"

#include <cereal/archives/json.hpp>

#include "test-to_testing_types.h"

using namespace hxcomm::vx;

template <class T>
class CommonUTMessageTests : public ::testing::Test
{};

typedef
    typename to_testing_types<instruction::ToFPGADictionary, instruction::FromFPGADictionary>::type
        UTMessageTypes;

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

	[[maybe_unused]] ut_message_t constructed_from_payload(config.get_payload());

	if constexpr (ut_message_t::payload_type::size) {
		auto payload =
		    hxcomm::random::random_payload<typename ut_message_t::instruction_type::Payload>();
		config.encode(payload);
		ASSERT_EQ(config.decode(), payload);

		[[maybe_unused]] ut_message_t constructed_from_instruction_payload(payload);

		ut_message_t config_eq = config;
		ut_message_t config_ne = config;
		config_ne.set_payload(~config_ne.get_payload());

		ASSERT_EQ(config, config_eq);
		ASSERT_FALSE(config == config_ne);

		ASSERT_NE(config, config_ne);
		ASSERT_FALSE(config != config_eq);
	}
}

TYPED_TEST(CommonUTMessageTests, Ostream)
{
	typename TypeParam::instruction_type::Payload payload;

	TypeParam message(payload);

	std::stringstream actual;
	actual << message;

	std::stringstream expected;
	expected << "UTMessage(raw: ";
	auto const words = message.get_raw().to_array();
	for (auto iter = words.rbegin(); iter != words.rend(); iter++) {
		std::stringstream ss;
		ss << std::hex << std::setfill('0') << std::setw(message.subword_width / 4)
		   << static_cast<hxcomm::largest_ut_message_subword_type>(*iter);
		expected << ss.str();
	}
	expected << ", " << payload << ")";

	EXPECT_EQ(actual.str(), expected.str());

	std::stringstream actual_payload;
	actual_payload << payload;

	std::stringstream expected_payload_prefix;
	expected_payload_prefix
	    << boost::typeindex::type_id<typename TypeParam::instruction_type>().pretty_name() << "(";

	// instruction type prefix
	EXPECT_EQ(
	    actual_payload.str().substr(0, expected_payload_prefix.str().length()),
	    expected_payload_prefix.str());

	// payload-value ostream representation is instruction type dependent and therefore not tested
	// here

	// payload-value postfix
	EXPECT_EQ(actual_payload.str().substr(actual_payload.str().length() - 1), ")");
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
