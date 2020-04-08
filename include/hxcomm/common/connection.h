#pragma once

#include <memory>
#include <type_traits>
#include <variant>

#include "hxcomm/common/to_utmessage_variant.h"

namespace hxcomm {

/**
 * Helper struct that defines message types from ConnectionParameters.
 *
 * @tparam ConnectionParameter UT message parameter for connection
 */
template <typename ConnectionParameter>
struct MessageTypes
{
	typedef typename hxcomm::ToUTMessageVariant<
	    ConnectionParameter::Send::HeaderAlignment,
	    typename ConnectionParameter::Send::SubwordType,
	    typename ConnectionParameter::Send::PhywordType,
	    typename ConnectionParameter::Send::Dictionary>::type send_type;

	typedef typename hxcomm::ToUTMessageVariant<
	    ConnectionParameter::Receive::HeaderAlignment,
	    typename ConnectionParameter::Receive::SubwordType,
	    typename ConnectionParameter::Receive::PhywordType,
	    typename ConnectionParameter::Receive::Dictionary>::type receive_type;

	typedef typename hxcomm::UTMessage<
	    ConnectionParameter::Send::HeaderAlignment,
	    typename ConnectionParameter::Send::SubwordType,
	    typename ConnectionParameter::Send::PhywordType,
	    typename ConnectionParameter::Send::Dictionary,
	    typename ConnectionParameter::SendHalt>
	    send_halt_type;
};

/**
 * Helper function to get MessageTypes from a variant over the same ConnectionParameter
 * in a streamlined fashion.
 */
template <typename C>
struct GetMessageTypes
{
	using type = typename C::message_types;
};

template <typename C>
struct GetMessageTypes<std::variant<C>>
{
	using type = typename GetMessageTypes<C>::type;
};

template <typename C>
struct GetMessageTypes<std::shared_ptr<C>>
{
	using type = typename GetMessageTypes<typename std::remove_cv<C>::type>::type;
};

template <typename... Cs>
struct GetMessageTypes<std::variant<Cs...>>
{
	using type = typename std::common_type<typename GetMessageTypes<Cs>::type...>::type;
};

} // namespace hxcomm
