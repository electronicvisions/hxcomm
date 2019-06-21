#include <gtest/gtest.h>

#include "hxcomm/common/utmessage.h"

#include "test-helper.h"

/** Return default-constructed UTMessage of runtime-specifiable header. */
template <typename UTMessageParameter>
struct default_message
{
	typedef typename hxcomm::ToUTMessageVariant<
	    UTMessageParameter::HeaderAlignment,
	    typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::PhywordType,
	    typename UTMessageParameter::Dictionary>::type message_type;

	template <size_t H, size_t... Hs>
	static message_type message_recurse(size_t header, std::index_sequence<H, Hs...>)
	{
		return (header == H) ? hxcomm::UTMessage<
		                           UTMessageParameter::HeaderAlignment,
		                           typename UTMessageParameter::SubwordType,
		                           typename UTMessageParameter::PhywordType,
		                           typename UTMessageParameter::Dictionary,
		                           typename hate::index_type_list_by_integer<
		                               H, typename UTMessageParameter::Dictionary>::type>()
		                     : message_recurse(header, std::index_sequence<Hs...>());
	}

	template <size_t H>
	static message_type message_recurse(size_t /*header*/, std::index_sequence<H>)
	{
		return hxcomm::UTMessage<
		    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType,
		    typename UTMessageParameter::PhywordType, typename UTMessageParameter::Dictionary,
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

template <typename UTMessageParameter>
typename default_message<UTMessageParameter>::message_type random_message()
{
	// random message type
	auto message = default_message<UTMessageParameter>::message(random_integer(
	    0, hate::type_list_size<typename UTMessageParameter::Dictionary>::value - 1));
	// random payload
	boost::apply_visitor([](auto& m) { m.set_payload(random_bitset<m.payload_width>()); }, message);
	return message;
}
