#pragma once

#include "hxcomm/vx/extollconnection.h"

#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({
	using ExtollConnection = ::hxcomm::vx::ExtollConnection;

	::pyhxcomm::ManagedPyBind11Helper<ExtollConnection>(
	    parent, BOOST_HANA_STRING("ExtollConnection"));
})

} // namespace pyhxcomm::vx
