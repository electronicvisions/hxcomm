#pragma once

#include <memory>
#include <type_traits>
#include <variant>

#include "hxcomm/common/target.h"
#include "hxcomm/common/to_utmessage_variant.h"

namespace hxcomm {

struct ConnectionTimeInfo;

#ifdef HXCOMM_HELPER_CHECK_MESSAGE_TYPE
#error "HXCOMM_HELPER_CHECK_MESSAGE_TYPE already defined"
#endif

#define HXCOMM_HELPER_CHECK_MESSAGE_TYPE(MESSAGE_TYPE)                                             \
	template <typename C, typename = void>                                                         \
	struct has_##MESSAGE_TYPE : std::false_type                                                    \
	{};                                                                                            \
                                                                                                   \
	template <typename C>                                                                          \
	struct has_##MESSAGE_TYPE<C, std::void_t<typename C::MESSAGE_TYPE>> : std::true_type           \
	{};                                                                                            \
                                                                                                   \
	template <typename C>                                                                          \
	constexpr static bool has_##MESSAGE_TYPE##_v = has_##MESSAGE_TYPE<C>::value;                   \
                                                                                                   \
	static_assert(has_##MESSAGE_TYPE##_v<Connection>, "Connection missing " #MESSAGE_TYPE ".");


/**
 * Base concept for all connections (ARQ, CoSim, QuiggeldyConnection, ...).
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
	struct has_get_time_info : std::false_type
	{};

	template <typename C>
	struct has_get_time_info<C, std::void_t<decltype(&C::get_time_info)>>
	{
		constexpr static bool value =
		    std::is_same_v<decltype(&C::get_time_info), ConnectionTimeInfo (C::*)() const>;
	};

	template <typename C>
	constexpr static bool has_get_time_info_v = has_get_time_info<C>::value;

	static_assert(has_get_time_info_v<Connection>, "Connection missing get_time_info-method.");

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

/**
 * Helper function to get MessageTypes from a Connection or a variant etc over the same
 * Connection in a streamlined fashion. If C is a ConnectionParameter
 * instance, the corresponding MessageTypes will be returned.
 */
template <typename C, typename = void>
struct GetMessageTypes
{
	using type = MessageTypes<std::decay_t<C>>;
};

template <typename C>
struct GetMessageTypes<C, std::void_t<typename C::message_types>>
{
	using type = typename C::message_types;
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
