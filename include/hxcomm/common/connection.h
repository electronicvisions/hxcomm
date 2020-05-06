#pragma once

#include <memory>
#include <type_traits>
#include <variant>

#include "hxcomm/common/target.h"
#include "hxcomm/common/to_utmessage_variant.h"

namespace hxcomm {

/**
 * Base concept for all connections (ARQ, CoSim, QuiggeldyClient, ...).
 */
template <typename Connection>
struct ConnectionConcept
{
	static_assert(
	    std::is_constructible_v<Connection, Connection&&>,
	    "Connection does not have a move constructor.");

	static_assert(
	    !std::is_constructible_v<Connection, Connection const&>,
	    "Connection has a copy constructor.");

	template <typename C, typename = void>
	struct has_supported_targets : std::false_type
	{};

	template <typename C>
	struct has_supported_targets<C, std::void_t<decltype(C::supported_targets)>>
	{
		constexpr static bool value =
		    std::is_same_v<decltype(Connection::supported_targets), std::initializer_list<Target>>;
	};

	template <typename C>
	constexpr static bool has_supported_targets_v = has_supported_targets<C>::value;

	static_assert(
	    has_supported_targets_v<Connection>, "Connection supported_targets list not declared.");
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
