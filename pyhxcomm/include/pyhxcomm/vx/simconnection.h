#pragma once

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/vx/simconnection.h"

#include "pyhxcomm/common/managed_connection.h"


namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {


GENPYBIND_MANUAL({
	using namespace boost::hana::literals;

	using SimConnection = ::hxcomm::vx::SimConnection;

	auto wrapper = ::pyhxcomm::ManagedPyBind11Helper<SimConnection>(parent, "SimConnection"_s);

	using handle_type = pyhxcomm::Handle<SimConnection>;

	wrapper.handle.def_property(
	    "enable_terminate_on_destruction",
	    [](handle_type& handle) -> bool {
		    return handle.get().get_enable_terminate_on_destruction();
	    },
	    [](handle_type& handle, bool value) {
		    handle.get().set_enable_terminate_on_destruction(value);
	    });
})

} // namespace pyhxcomm::vx
