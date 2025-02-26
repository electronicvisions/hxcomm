#pragma once

#include "pyhxcomm/common/handle_connection.h"
#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/connection_handle_binding.h"
#include "pyhxcomm/vx/genpybind.h"

#include "hxcomm/vx/quiggeldy_connection.h"

#include "pybind11/chrono.h"

#include <chrono>

namespace pyhxcomm { namespace vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({
	using QuiggeldyConnection = ::hxcomm::vx::QuiggeldyConnection;

	auto wrapped = ::pyhxcomm::ManagedPyBind11Helper<QuiggeldyConnection>(
	    parent, BOOST_HANA_STRING("QuiggeldyConnection"));

	using handle_type = ::pyhxcomm::Handle<QuiggeldyConnection>;

	wrapped.handle
	    .def_property(
	        "use_munge", [](handle_type& handle) -> bool { return handle.get().get_use_munge(); },
	        [](handle_type& handle, bool value) { handle.get().set_use_munge(value); })
	    .def_property(
	        "connection_attempts_max",
	        [](handle_type& handle) -> size_t {
		        return handle.get().get_connection_attempts_max();
	        },
	        [](handle_type& handle, size_t value) {
		        handle.get().set_connection_attempts_max(value);
	        })
	    .def_property(
	        "connection_attempt_wait_after",
	        [](handle_type& handle) -> std::chrono::milliseconds {
		        return handle.get().get_connection_attempt_wait_after();
	        },
	        [](handle_type& handle, std::chrono::milliseconds value) {
		        handle.get().set_connection_attempt_wait_after(value);
	        })
	    .def_property(
	        "out_of_order",
	        [](handle_type& handle) -> bool { return handle.get().is_out_of_order(); },
	        [](handle_type& handle, bool set_out_of_order) {
		        auto& conn = handle.get();
		        // Since out_of_order can only be set per connection but not
		        // reset, we need to perform some checks in order to emulate a
		        // Python property.
		        if (conn.is_out_of_order() == set_out_of_order) {
			        // value to be set corresponds to current-state -> fine
		        } else if (set_out_of_order) {
			        // If user requests out-of-order execution we can comply as well.
			        conn.set_out_of_order();
		        } else {
			        // User requests in-order execution but had previously
			        // requested out-of-order execution -> error.
			        throw std::runtime_error("Cannot switch back to in-order execution after "
			                                 "having executed out-of-order.");
		        }
	        });
})

} // namespace vx
} // namespace pyhxcomm
