#pragma once

#include "pyhxcomm/common/handle_connection.h"
#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/connection_handle_binding.h"
#include "pyhxcomm/vx/genpybind.h"

#include "hxcomm/vx/quiggeldy_connection.h"

#include "pybind11/chrono.h"

#include <chrono>

namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

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
	        });
})

} // namespace pyhxcomm::vx
