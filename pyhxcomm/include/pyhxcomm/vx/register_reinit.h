#pragma once

#include "hxcomm/vx/register_reinit.h"
#include "pyhxcomm/vx/connection_handle.h"

#include <type_traits>
#include <pybind11/pybind11.h>

namespace pyhxcomm::vx {

namespace detail {

/**
 * Simple helper to unroll templated constructor of RegisterReinit over the
 * whole connection variant.
 *
 * Used in fisch and stadls.
 */
template <typename Wrapped, typename>
struct register_reinit_unroll_helper
{
	using wrapped_t = Wrapped;
	register_reinit_unroll_helper(wrapped_t&) {}
};

template <typename Wrapped, typename ConnectionHandle, typename... Rest>
struct register_reinit_unroll_helper<Wrapped, std::variant<ConnectionHandle, Rest...>>
    : register_reinit_unroll_helper<Wrapped, std::variant<Rest...>>
{
	using parent_t = register_reinit_unroll_helper<Wrapped, std::variant<Rest...>>;
	using wrapped_t = typename parent_t::wrapped_t;

	constexpr register_reinit_unroll_helper(wrapped_t& wrapped) : parent_t({wrapped})
	{
		wrapped.def(pybind11::init([](ConnectionHandle& handle) -> typename wrapped_t::type {
			typename ConnectionHandle::connection_type const& connection = handle.get();
			return typename wrapped_t::type(connection);
		}));
	}
};

} // namespace detail

template <typename Wrapped>
using register_reinit_unroll_helper =
    typename detail::register_reinit_unroll_helper<Wrapped, pyhxcomm::vx::ConnectionHandle>;

} // namespace pyhxcomm::vx
