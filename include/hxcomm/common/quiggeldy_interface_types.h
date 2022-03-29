#pragma once

#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/execute_messages.h"

#include <optional>
#include <utility>
#include <vector>

namespace hxcomm {

namespace detail {

template <typename ConnectionParameter>
struct ReinitEntryType
{
	using request_type = detail::execute_messages_argument_t<ConnectionParameter>;

	request_type request;

	std::optional<request_type> snapshot;
	typedef typename ConnectionParameter::QuiggeldyScheduleOutToInTransform transform_type;
};

} // namespace detail

template <typename ConnectionParameter>
struct quiggeldy_interface_types
{
	using message_types = MessageTypes<ConnectionParameter>;

	using request_type = detail::execute_messages_argument_t<ConnectionParameter>;
	using response_type = detail::execute_messages_return_t<ConnectionParameter>;
	using reinit_entry_type = detail::ReinitEntryType<ConnectionParameter>;
	using reinit_type = std::vector<reinit_entry_type>;
};

// convenience partial specialization if only message types are available
template <typename ConnectionParameter>
struct quiggeldy_interface_types<MessageTypes<ConnectionParameter>>
    : quiggeldy_interface_types<ConnectionParameter>
{};

} // namespace hxcomm
