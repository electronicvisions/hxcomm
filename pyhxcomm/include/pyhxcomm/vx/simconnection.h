#pragma once

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/vx/multi_simconnection.h"

#include "pyhxcomm/common/managed_connection.h"


namespace pyhxcomm {
namespace vx GENPYBIND_TAG_HXCOMM_VX {


GENPYBIND_MANUAL({
	using namespace boost::hana::literals;

	using MultiSimConnection = ::hxcomm::vx::MultiSimConnection;

	auto wrapper =
	    ::pyhxcomm::ManagedPyBind11Helper<MultiSimConnection>(parent, "MultiSimConnection"_s);

	using handle_type = pyhxcomm::Handle<MultiSimConnection>;

	wrapper.handle.def_property(
	    "enable_terminate_on_destruction",
	    [](handle_type& handle) -> std::vector<bool> {
		    std::vector<bool> ret;
		    for (size_t i = 0; i < handle.get().size(); i++) {
			    ret.push_back(handle.get()[i].get_enable_terminate_on_destruction());
		    }
		    return ret;
	    },
	    [](handle_type& handle, std::vector<bool> value) {
		    if (value.size() != handle.get().size()) {
			    throw std::invalid_argument(
			        "Number of given values does not match number of connections.");
		    }
		    for (size_t i = 0; i < value.size(); i++) {
			    handle.get()[i].set_enable_terminate_on_destruction(value[i]);
		    }
	    });
})

} // namespace vx
} // namespace pyhxcomm
