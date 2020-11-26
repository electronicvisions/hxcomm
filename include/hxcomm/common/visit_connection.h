#pragma once

#include <type_traits>
#include <variant>
#include <hate/type_traits.h> // for some gcc 8 hacks

namespace hxcomm {

/**
 * Helper struct that is used to differentiate between plain connections and
 * connections wrapped up in second order types (e.g. variant, Handles).
 *
 * Used in `stadls::run` and `ConnectionIs{Plain,Wrapped}Guard` below.
 */
template <typename Connection>
struct ConnectionIsPlain : std::true_type
{};

template <typename... Connections>
struct ConnectionIsPlain<std::variant<Connections...>> : std::false_type
{};

template <typename Connection>
struct ConnectionIsPlain<std::shared_ptr<Connection>> : std::false_type
{};

namespace detail {

template <typename Visitor, typename Connection>
struct VisitConnection
{
	using return_type = decltype(std::declval<Visitor>()(std::declval<Connection>()));

	return_type operator()(Visitor&& visitor, Connection&& connection)
	{
		return visitor(std::forward<Connection>(connection));
	}
};

template <typename Visitor, typename Connection>
struct VisitConnection<Visitor, std::shared_ptr<Connection>&>
{
	using return_type = decltype(std::declval<Visitor>()(std::declval<Connection&>()));

	return_type operator()(Visitor&& visitor, std::shared_ptr<Connection> const& connection)
	{
		return visitor(*connection);
	}
};

template <typename Visitor, typename... Connections>
struct VisitConnection<Visitor, std::variant<Connections...>&>
{
	using variant_type = std::variant<Connections...>;
	using return_type = typename std::common_type<
	    typename VisitConnection<Visitor, Connections&>::return_type...>::type;

	return_type operator()(Visitor&& visitor, variant_type& variant)
	{
		return std::visit(
		    [&visitor](auto&& connection) -> return_type {
			    return VisitConnection<Visitor, decltype(connection)&>()(
			        std::forward<Visitor>(visitor), connection);
		    },
		    variant);
	}
};

} // namespace detail

/**
 * Indicate whether supplied connection type is a plain Connection object
 * that can be acted upon.
 *
 * It is intended to be used as a defaulted template argument and needs a
 * default in parameter (e.g. 0).
 */
template <typename Connection>
using ConnectionIsPlainGuard =
    std::enable_if_t<ConnectionIsPlain<std::remove_cvref_t<Connection>>::value, int>;

/**
 * Indicate whether supplied connection type is wrapped up and needs to be
 * unwrapped in some kind (e.g. in a variant that needs to be visited).
 *
 * It is intended to be used as a defaulted template argument and needs a
 * default in parameter (e.g. 0).
 */
template <typename Connection>
using ConnectionIsWrappedGuard =
    std::enable_if_t<!ConnectionIsPlain<std::remove_cvref_t<Connection>>::value, int>;

/**
 * Vist a connection that might be wrapped inside a variant or proxified.
 *
 * @param visitor must be a Callable with templated operator() so that it can
 * be applied to the extracted connection underneath.

 * @param connection The connection to visit.
 */
template <typename Visitor, typename Connection>
typename detail::VisitConnection<Visitor, Connection>::return_type visit_connection(
    Visitor&& visitor, Connection&& connection)
{
	return detail::VisitConnection<Visitor, Connection>()(
	    std::forward<Visitor>(visitor), std::forward<Connection>(connection));
}


} // namespace hxcomm
