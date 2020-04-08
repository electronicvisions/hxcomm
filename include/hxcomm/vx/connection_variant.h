#pragma once

#include "hxcomm/common/connection_variant.h"

#include "hxcomm/vx/connection_parameter.h"
#include "hxcomm/vx/genpybind.h"

namespace hxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

typedef hxcomm::ConnectionVariantType<hxcomm::vx::ConnectionParameter> ConnectionVariantType;

GENPYBIND_MANUAL({
	parent->py::template class_<::hxcomm::vx::ConnectionVariantType>(
	    parent, "Connection");
})

} // namespace hxcomm::vx

#ifdef __GENPYBIND__
// We need to import the full python bindings here, otherwise the variant types
// will not be identified with their pyhxcomm-binding counterparts when
// included in other layers (e.g. fisch, haldls).
#include "hxcomm/vx/python_bindings.h"
#endif
