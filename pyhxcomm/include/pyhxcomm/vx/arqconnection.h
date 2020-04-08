#pragma once

#include "hxcomm/vx/arqconnection.h"

#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

GENPYBIND_MANUAL({
	using ARQConnection = ::hxcomm::vx::ARQConnection;

	::pyhxcomm::ManagedPyBind11Helper<ARQConnection>(parent, BOOST_HANA_STRING("ARQConnection"));
})

} // namespace pyhxcomm::vx
