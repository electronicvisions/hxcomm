#pragma once
#include "pyhxcomm/vx/genpybind.h"

#include <pybind11/chrono.h>
#include <pybind11/pybind11.h>

#include "hxcomm/common/connection_time_info.h"

namespace pyhxcomm { namespace vx GENPYBIND_TAG_HXCOMM_VX {

typedef hxcomm::ConnectionTimeInfo ConnectionTimeInfo GENPYBIND(opaque);

GENPYBIND_MANUAL({
	auto attr = parent.attr("ConnectionTimeInfo");
	auto ism = parent->py::is_method(attr);
	auto const str = [](::pyhxcomm::vx::ConnectionTimeInfo const& data) -> std::string {
		std::stringstream ss;
		ss << data;
		return ss.str();
	};
	attr.attr("__str__") = parent->py::cpp_function(str, ism);
	attr.attr("__repr__") = parent->py::cpp_function(str, ism);
})

} // namespace vx
} // namespace pyhxcomm
