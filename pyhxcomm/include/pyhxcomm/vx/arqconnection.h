#pragma once

#include "hxcomm/vx/multi_arqconnection.h"

#include "pyhxcomm/common/managed_connection.h"
#include "pyhxcomm/vx/genpybind.h"

namespace pyhxcomm {
namespace vx GENPYBIND_TAG_HXCOMM_VX {
#ifdef WITH_HXCOMM_HOSTARQ
GENPYBIND_MANUAL({
	using MultiARQConnection = ::hxcomm::vx::MultiARQConnection;

	::pyhxcomm::ManagedPyBind11Helper<MultiARQConnection>(
	    parent, BOOST_HANA_STRING("MultiARQConnection"));
})
#endif

} // namespace vx
} // namespace pyhxcomm
