#pragma once

#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/execute_messages.h"

#include <utility>
#include <vector>

namespace hxcomm {

template <typename ConnectionParameter>
struct quiggeldy_interface_types
{
	using message_types = MessageTypes<ConnectionParameter>;

	using request_type = detail::execute_messages_argument_t<ConnectionParameter>;
	using response_type = detail::execute_messages_return_t<ConnectionParameter>;
	using reinit_type = std::vector<request_type>;
	using reinit_entry_type = typename reinit_type::value_type;
};

// convenience partial specialization if only message types are available
template <typename ConnectionParameter>
struct quiggeldy_interface_types<MessageTypes<ConnectionParameter>>
    : quiggeldy_interface_types<ConnectionParameter>
{};

} // namespace hxcomm
