#pragma once

#include <vector>

#include "hxcomm/common/target_restriction.h"
#include "hxcomm/common/to_utmessage_variant.h"

namespace hxcomm {

#ifdef HXCOMM_HELPER_CHECK_MESSAGE_TYPE
#error "HXCOMM_HELPER_CHECK_MESSAGE_TYPE already defined"
#endif

#define HXCOMM_HELPER_CHECK_MESSAGE_TYPE(MESSAGE_TYPE)                                             \
	static_assert(                                                                                 \
	    sizeof(typename Connection::MESSAGE_TYPE) > 0, "Connection missing " #MESSAGE_TYPE);


/**
 * Base concept for all connections (ARQ, CoSim, QuiggeldyClient, ...).
 */
template <typename Connection>
struct ConnectionConcept
{
	static_assert(
	    std::is_same_v<decltype(Connection(std::declval<Connection&&>())), Connection>,
	    "Connection does not have a move constructor.");

	static_assert(
	    std::is_same_v<
	        decltype(&Connection::supports),
	        bool (Connection::*)(TargetRestriction) const>,
	    "Connection supports method not declared.");

	HXCOMM_HELPER_CHECK_MESSAGE_TYPE(send_message_type)
	HXCOMM_HELPER_CHECK_MESSAGE_TYPE(receive_message_type)
	HXCOMM_HELPER_CHECK_MESSAGE_TYPE(send_halt_message_type)
};

#undef HXCOMM_HELPER_CHECK_MESSAGE_TYPE

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

#ifdef HXCOMM_EXPOSE_MESSAGE_TYPES
#error "HXCOMM_EXPOSE_MESSAGE_TYPES already defined"
#else
/**
 * Helper macro that re-exposes all message types from MessageTypes in the current connection.
 */
#define HXCOMM_EXPOSE_MESSAGE_TYPES(CONN_PARAMS)                                                   \
	using message_types = MessageTypes<CONN_PARAMS>;                                               \
                                                                                                   \
	using receive_message_type = typename message_types::receive_type;                             \
	using send_message_type = typename message_types::send_type;                                   \
	using send_halt_message_type = typename message_types::send_halt_type;
#endif


} // namespace hxcomm
