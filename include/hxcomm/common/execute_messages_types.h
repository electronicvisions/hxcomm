#pragma once

#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"

#include "hate/type_traits.h"

#include <type_traits>
#include <vector>

namespace hxcomm::detail {

template <typename Connection>
struct ExecuteMessagesReturnType
{
	using type = std::pair<
	    std::vector<typename GetMessageTypes<std::remove_cvref_t<Connection>>::type::receive_type>,
	    ConnectionTimeInfo>;
};

template <typename Connection>
using execute_messages_return_t = typename ExecuteMessagesReturnType<Connection>::type;

template <typename Connection>
struct ExecuteMessagesArgumentType
{
	using type =
	    std::vector<typename GetMessageTypes<std::remove_cvref_t<Connection>>::type::send_type>;
};

template <typename Connection>
using execute_messages_argument_t = typename ExecuteMessagesArgumentType<Connection>::type;

} // namespace hxcomm::detail
