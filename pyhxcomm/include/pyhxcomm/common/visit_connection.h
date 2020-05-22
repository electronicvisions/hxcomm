#pragma once

#include <type_traits>
#include <variant>

#include "hxcomm/common/visit_connection.h"
#include "pyhxcomm/common/handle_connection.h"

namespace hxcomm {

template <typename Connection>
struct ConnectionIsPlain<pyhxcomm::Handle<Connection>> : std::false_type
{};

namespace detail {

template <typename Visitor, typename Connection>
struct VisitConnection<Visitor, pyhxcomm::Handle<Connection>&>
{
	using handle_type = pyhxcomm::Handle<Connection>;
	using connection_type = Connection&;
	using return_type = typename VisitConnection<Visitor, connection_type>::return_type;

	return_type operator()(Visitor&& visitor, handle_type& handle)
	{
		return VisitConnection<Visitor, connection_type>()(
		    std::forward<Visitor>(visitor), handle.get());
	}
};

template <typename Visitor, typename Connection>
struct VisitConnection<Visitor, pyhxcomm::Handle<Connection> const&>
{
	using handle_type = pyhxcomm::Handle<Connection>;
	using connection_type = Connection const&;
	using return_type = typename VisitConnection<Visitor, connection_type>::return_type;

	return_type operator()(Visitor&& visitor, handle_type const& handle)
	{
		return VisitConnection<Visitor, connection_type>()(
		    std::forward<Visitor>(visitor), handle.get());
	}
};

} // namespace detail

} // namespace hxcomm
