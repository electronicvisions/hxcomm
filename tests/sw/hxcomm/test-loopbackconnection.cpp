#include <gtest/gtest.h>

#include "hxcomm/common/connection_parameter.h"
#include "hxcomm/common/loopbackconnection.h"
#include "hxcomm/vx/utmessage.h"

#include "test-helper.h"

using namespace hxcomm;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

/** Return default-constructed ut_message of runtime-specifiable header. */
template <typename UTMessageParameter, typename SubpacketType>
struct default_message
{
	typedef typename LoopbackConnection<UTMessageParameter, SubpacketType>::receive_message_type
	    message_type;

	template <size_t H, size_t... Hs>
	static message_type message_recurse(size_t header, std::index_sequence<H, Hs...>)
	{
		return (header == H) ? ut_message<
		                           UTMessageParameter::HeaderAlignment,
		                           typename UTMessageParameter::SubwordType,
		                           typename UTMessageParameter::Dictionary,
		                           typename hate::index_type_list_by_integer<
		                               H, typename UTMessageParameter::Dictionary>::type>()
		                     : message_recurse(header, std::index_sequence<Hs...>());
	}

	template <size_t H>
	static message_type message_recurse(size_t /*header*/, std::index_sequence<H>)
	{
		return ut_message<
		    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType,
		    typename UTMessageParameter::Dictionary,
		    typename hate::index_type_list_by_integer<
		        H, typename UTMessageParameter::Dictionary>::type>();
	}

	static message_type message(size_t header)
	{
		return message_recurse(
		    header, std::make_index_sequence<
		                hate::type_list_size<typename UTMessageParameter::Dictionary>::value>());
	}
};

template <class LoopbackConnectionType>
typename LoopbackConnectionType::send_message_type random_message()
{
	// random message type
	auto message = default_message<
	    UTMessageParameter<
	        LoopbackConnectionType::header_alignment, typename LoopbackConnectionType::subword_type,
	        typename LoopbackConnectionType::dictionary_type>,
	    typename LoopbackConnectionType::subpacket_type>::
	    message(random_integer(
	        0, hate::type_list_size<typename LoopbackConnectionType::dictionary_type>::value - 1));
	// random payload
	boost::apply_visitor([](auto& m) { m.set_payload(random_bitset<m.payload_width>()); }, message);
	return message;
}

constexpr std::array<size_t, 3> header_alignments{1, 3, 8};

template <class T>
struct to_testing_types;

template <size_t... N>
struct to_testing_types<std::index_sequence<N...> >
{
	typedef ::testing::Types<
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint8_t, to_fpga_dictionary>,
	        uint8_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint16_t, to_fpga_dictionary>,
	        uint8_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint32_t, to_fpga_dictionary>,
	        uint8_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint64_t, to_fpga_dictionary>,
	        uint8_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint8_t, to_fpga_dictionary>,
	        uint16_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint16_t, to_fpga_dictionary>,
	        uint16_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint32_t, to_fpga_dictionary>,
	        uint16_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint64_t, to_fpga_dictionary>,
	        uint16_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint8_t, to_fpga_dictionary>,
	        uint32_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint16_t, to_fpga_dictionary>,
	        uint32_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint32_t, to_fpga_dictionary>,
	        uint32_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint64_t, to_fpga_dictionary>,
	        uint32_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint8_t, to_fpga_dictionary>,
	        uint64_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint16_t, to_fpga_dictionary>,
	        uint64_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint32_t, to_fpga_dictionary>,
	        uint64_t>...,
	    LoopbackConnection<
	        UTMessageParameter<header_alignments[N], uint64_t, to_fpga_dictionary>,
	        uint64_t>...>
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
	auto message = random_message<TypeParam>();

	TypeParam connection;
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
	auto message = random_message<TypeParam>();

	TypeParam connection;
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
	auto message = random_message<TypeParam>();

	TypeParam connection;
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

	std::vector<typename TypeParam::send_message_type> messages;
	for (size_t i = 0; i < message_count; ++i) {
		auto message = random_message<TypeParam>();
		messages.push_back(message);
	}

	TypeParam connection;
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
