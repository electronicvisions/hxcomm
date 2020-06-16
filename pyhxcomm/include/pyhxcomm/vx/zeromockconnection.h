#pragma once

#include "hxcomm/vx/zeromockconnection.h"

#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({
	using ZeroMockConnection = ::hxcomm::vx::ZeroMockConnection;

	::pyhxcomm::ManagedPyBind11Helper<ZeroMockConnection>(
	    parent, BOOST_HANA_STRING("ZeroMockConnection"));
})

} // namespace pyhxcomm::vx
