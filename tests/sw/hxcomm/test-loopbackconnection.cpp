#include <gtest/gtest.h>

#include "hxcomm/common/connection_parameter.h"
#include "hxcomm/common/loopbackconnection.h"
#include "hxcomm/vx/utmessage.h"
#include "hxcomm/vx/utmessage_random.h"

#include "test-helper.h"

using namespace hxcomm;
using namespace hxcomm::random;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

constexpr std::array<size_t, 3> header_alignments{1, 3, 8};

template <class T>
struct to_testing_types;

template <size_t... N>
struct to_testing_types<std::index_sequence<N...> >
{
	typedef ::testing::Types<
	    UTMessageParameter<header_alignments[N], uint8_t, uint8_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint16_t, uint8_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint32_t, uint8_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint64_t, uint8_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint8_t, uint16_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint16_t, uint16_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint32_t, uint16_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint64_t, uint16_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint8_t, uint32_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint16_t, uint32_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint32_t, uint32_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint64_t, uint32_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint8_t, uint64_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint16_t, uint64_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint32_t, uint64_t, ToFPGADictionary>...,
	    UTMessageParameter<header_alignments[N], uint64_t, uint64_t, ToFPGADictionary>...>
	    types;
};

typedef to_testing_types<std::make_index_sequence<header_alignments.size()> >::types Types;

template <class T>
class CommonLoopbackConnectionTests : public ::testing::Test
{};

TYPED_TEST_CASE(CommonLoopbackConnectionTests, Types);

constexpr auto empty_sleep = std::chrono::microseconds(10000);

TYPED_TEST(CommonLoopbackConnectionTests, OneMessage)
{
	auto message = random_ut_message<TypeParam>();

	LoopbackConnection<TypeParam> connection;
	boost::apply_visitor([&connection](auto m) { connection.add(m); }, message);
	connection.commit();
	while (connection.receive_empty()) {
		std::this_thread::sleep_for(empty_sleep);
	}
	auto message_return = connection.receive();
	EXPECT_EQ(message, message_return);
}

TYPED_TEST(CommonLoopbackConnectionTests, OneMessageAfterCommit)
{
	auto message = random_ut_message<TypeParam>();

	LoopbackConnection<TypeParam> connection;
	connection.commit();
	boost::apply_visitor([&connection](auto m) { connection.add(m); }, message);
	connection.commit();
	while (connection.receive_empty()) {
		std::this_thread::sleep_for(empty_sleep);
	}
	auto message_return = connection.receive();
	EXPECT_EQ(message, message_return);
}

TYPED_TEST(CommonLoopbackConnectionTests, OneMessageMultiCommit)
{
	auto message = random_ut_message<TypeParam>();

	LoopbackConnection<TypeParam> connection;
	boost::apply_visitor([&connection](auto m) { connection.add(m); }, message);
	connection.commit();
	connection.commit();
	while (connection.receive_empty()) {
		std::this_thread::sleep_for(empty_sleep);
	}
	auto message_return = connection.receive();
	EXPECT_EQ(message, message_return);
}

TYPED_TEST(CommonLoopbackConnectionTests, MultipleMessages)
{
	constexpr size_t max_message_count = 100;
	size_t const message_count = random_integer(0, max_message_count);

	std::vector<typename LoopbackConnection<TypeParam>::send_message_type> messages;
	for (size_t i = 0; i < message_count; ++i) {
		auto message = random_ut_message<TypeParam>();
		messages.push_back(message);
	}

	LoopbackConnection<TypeParam> connection;
	connection.add(messages);
	connection.commit();
	for (size_t i = 0; i < message_count; ++i) {
		while (connection.receive_empty()) {
			std::this_thread::sleep_for(empty_sleep);
		}
		auto message_return = connection.receive();
		EXPECT_EQ(message_return, messages.at(i));
	}
	// no more messages available to receive
	ASSERT_THROW(connection.receive(), std::runtime_error);
}
