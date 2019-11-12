#include <gtest/gtest.h>

#include "hxcomm/common/payload_random.h"
#include "hxcomm/common/utmessage.h"

namespace hxcomm::random {

/**
 * Return default-constructed UTMessage of runtime-specifiable header.
 */
template <typename UTMessageParameter>
struct default_ut_message
{
public:
	typedef typename hxcomm::ToUTMessageVariant<
	    UTMessageParameter::HeaderAlignment,
	    typename UTMessageParameter::SubwordType,
	    typename UTMessageParameter::PhywordType,
	    typename UTMessageParameter::Dictionary>::type message_type;

private:
	template <size_t H, size_t... Hs>
	static message_type message_recurse(size_t const header, std::index_sequence<H, Hs...>)
	{
		return (header == H) ? UTMessage<
		                           UTMessageParameter::HeaderAlignment,
		                           typename UTMessageParameter::SubwordType,
		                           typename UTMessageParameter::PhywordType,
		                           typename UTMessageParameter::Dictionary,
		                           typename hate::index_type_list_by_integer<
		                               H, typename UTMessageParameter::Dictionary>::type>()
		                     : message_recurse(header, std::index_sequence<Hs...>());
	}

	template <size_t H>
	static message_type message_recurse(size_t const header, std::index_sequence<H>)
	{
		if (header != H) {
			throw std::logic_error("Header number not present in dictionary.");
		}
		return UTMessage<
		    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType,
		    typename UTMessageParameter::PhywordType, typename UTMessageParameter::Dictionary,
		    typename hate::index_type_list_by_integer<
		        H, typename UTMessageParameter::Dictionary>::type>();
	}

public:
	static message_type get(size_t const header)
	{
		return message_recurse(
		    header, std::make_index_sequence<
		                hate::type_list_size<typename UTMessageParameter::Dictionary>::value>());
	}
};

/**
 * Generate a random UTMessage of the provided dictionary with random payload.
 * @tparam UTMessageParameter Parameter of UTMessage collection
 * @param gen Random number generator
 * @return Random UT message with random payload
 */
template <typename UTMessageParameter>
auto random_ut_message(std::mt19937 gen = std::mt19937(std::random_device{}())) ->
    typename default_ut_message<UTMessageParameter>::message_type
{
	std::uniform_int_distribution<size_t> random_header(
	    0, hate::type_list_size<typename UTMessageParameter::Dictionary>::value - 1);

	// random default message
	auto message = default_ut_message<UTMessageParameter>::get(random_header(gen));
	// random payload
	boost::apply_visitor(
	    [&gen](auto& m) {
		    m.encode(random_payload<
		             typename std::remove_reference<decltype(m)>::type::instruction_type::Payload>(
		        gen));
	    },
	    message);
	return message;
}

} // namespace hxcomm::random
