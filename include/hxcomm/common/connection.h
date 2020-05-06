#pragma once

#include <vector>

#include "hxcomm/common/target_restriction.h"
#include "hxcomm/common/to_utmessage_variant.h"

namespace hxcomm {

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
};

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

} // namespace hxcomm
