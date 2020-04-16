#include <gtest/gtest.h>

#include "hxcomm/common/connection_parameter.h"
#include "hxcomm/common/loopbackconnection.h"
#include "hxcomm/vx/utmessage.h"
#include "hxcomm/vx/utmessage_random.h"

#include "hxcomm/test-helper.h"

using namespace hxcomm;
using namespace hxcomm::random;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;
using namespace hxcomm::test;

#ifndef SUBWORD_TYPE
#define SUBWORD_TYPE uint64_t
#endif

#ifndef PHYWORD_TYPE
#define PHYWORD_TYPE uint64_t
#endif


#ifndef HEADER_ALIGNMENT
#define HEADER_ALIGNMENT 8
#endif

typedef UTMessageParameter<HEADER_ALIGNMENT, SUBWORD_TYPE, PHYWORD_TYPE, ToFPGADictionary>
    TestUTMessageParameter;

constexpr auto empty_sleep = std::chrono::microseconds(10000);

/*
 * multiple nested macros needed to first unroll type and alignment defines to their value instead
 * of their name and afterwards concatenate the values to the complete name in order to get
 * something like CommonLoopbackConnectionTests_uint8_t_uint64_t_3.
 */
#define NAME__(A, B, C) CommonLoopbackConnectionTests_##A##_##B##_##C
#define NAME_(A, B, C) NAME__(A, B, C)
#define Name NAME_(SUBWORD_TYPE, PHYWORD_TYPE, HEADER_ALIGNMENT)

/** Nested macro needed to unroll value of ClassName define from above instead of define name.  */
#define MYTEST(ClassName, CaseName) TEST(ClassName, CaseName)

MYTEST(Name, OneMessage)
{
	auto message = random_ut_message<TestUTMessageParameter>();

	LoopbackConnection<TestUTMessageParameter> connection;
	boost::apply_visitor([&connection](auto m) { connection.add(m); }, message);
	connection.commit();
	while (connection.receive_empty()) {
		std::this_thread::sleep_for(empty_sleep);
	}
	auto message_return = connection.receive();
	EXPECT_EQ(message, message_return);
}

MYTEST(Name, OneMessageAfterCommit)
{
	auto message = random_ut_message<TestUTMessageParameter>();

	LoopbackConnection<TestUTMessageParameter> connection;
	connection.commit();
	boost::apply_visitor([&connection](auto m) { connection.add(m); }, message);
	connection.commit();
	while (connection.receive_empty()) {
		std::this_thread::sleep_for(empty_sleep);
	}
	auto message_return = connection.receive();
	EXPECT_EQ(message, message_return);
}

MYTEST(Name, OneMessageMultiCommit)
{
	auto message = random_ut_message<TestUTMessageParameter>();

	LoopbackConnection<TestUTMessageParameter> connection;
	boost::apply_visitor([&connection](auto m) { connection.add(m); }, message);
	connection.commit();
	connection.commit();
	while (connection.receive_empty()) {
		std::this_thread::sleep_for(empty_sleep);
	}
	auto message_return = connection.receive();
	EXPECT_EQ(message, message_return);
}

MYTEST(Name, MultipleMessages)
{
	constexpr size_t max_message_count = 100;
	size_t const message_count = random_integer(0, max_message_count);

	std::vector<typename LoopbackConnection<TestUTMessageParameter>::send_message_type> messages;
	for (size_t i = 0; i < message_count; ++i) {
		auto message = random_ut_message<TestUTMessageParameter>();
		messages.push_back(message);
	}

	LoopbackConnection<TestUTMessageParameter> connection;
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
