#pragma once

#include "hxcomm/vx/reinit_stack_entry.h"
#include "pyhxcomm/vx/connection_handle.h"
#include "pyhxcomm/vx/genpybind.h"

#include <type_traits>
#include <pybind11/pybind11.h>

namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

namespace detail {

/**
 * Simple helper to unroll templated constructor of RegisterReinit over the
 * whole connection variant.
 *
 * Used in pyfisch and pystadls.
 */
template <typename Wrapped, typename>
struct reinit_stack_entry_unroll_helper
{
	using wrapped_t = Wrapped;
	reinit_stack_entry_unroll_helper(wrapped_t&) {}
};

template <typename Wrapped, typename ConnectionHandle, typename... Rest>
struct reinit_stack_entry_unroll_helper<Wrapped, std::variant<ConnectionHandle, Rest...>>
    : reinit_stack_entry_unroll_helper<Wrapped, std::variant<Rest...>>
{
	using parent_t = reinit_stack_entry_unroll_helper<Wrapped, std::variant<Rest...>>;
	using wrapped_t = typename parent_t::wrapped_t;

	constexpr reinit_stack_entry_unroll_helper(wrapped_t& wrapped) : parent_t({wrapped})
	{
		wrapped.def(pybind11::init([](ConnectionHandle & handle) -> typename wrapped_t::type {
			typename ConnectionHandle::connection_type& connection = handle.get();
			return typename wrapped_t::type{connection};
		}));
	}
};

} // namespace detail

template <typename Wrapped>
using reinit_stack_entry_unroll_helper =
    typename detail::reinit_stack_entry_unroll_helper<Wrapped, pyhxcomm::vx::ConnectionHandle>;

GENPYBIND_MANUAL({
	pybind11::class_<::hxcomm::vx::ReinitStackEntry> wrapped(parent, "ReinitStackEntry");
	using wrapped_t = decltype(wrapped);

	[[maybe_unused]] ::pyhxcomm::vx::reinit_stack_entry_unroll_helper<wrapped_t> helper{wrapped};

	using reinit_stack_entry_t = typename wrapped_t::type;
	using reinit_entry_type = typename reinit_stack_entry_t::reinit_entry_type;

	void (reinit_stack_entry_t::*call_ref)(reinit_entry_type const&, bool) =
	    &reinit_stack_entry_t::set;

	wrapped.def("pop", &::hxcomm::vx::ReinitStackEntry::pop);
	wrapped.def("set", call_ref, pybind11::arg("entry"), pybind11::arg("enforce") = true);
})

} // namespace pyhxcomm::vxGENPYBIND_TAG_HXCOMM_VX
