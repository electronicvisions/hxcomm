#pragma once

#include "hxcomm/vx/multi_zeromockconnection.h"

#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm {
namespace vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({
	using MultiZeroMockConnection = ::hxcomm::vx::MultiZeroMockConnection;

	::pyhxcomm::ManagedPyBind11Helper<MultiZeroMockConnection>(
	    parent, BOOST_HANA_STRING("MultiZeroMockConnection"));
})

} // namespace vx
} // namespace pyhxcomm
