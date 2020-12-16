#pragma once

#include "cereal/types/utility.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/types/vector.hpp"

#include "hxcomm/common/cerealization_connection_time_info.h"
#include "hxcomm/common/cerealization_utmessage.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/sf_serialization.h"
#include "hxcomm/vx/connection_parameter.h"
#include "hxcomm/vx/quiggeldy_interface_types.h"

#include "rcf-extensions/round-robin-reinit-scheduler.h"
#include "rcf-extensions/sf/optional.h"

#include <optional>
#include <RCF/RCF.hpp>

namespace hxcomm::vx {

class I_HXCommQuiggeldyVX;

} // namespace hxcomm::vx

namespace SF {

inline void serialize(Archive& ar, hxcomm::vx::quiggeldy_interface_types::request_type& qcr)
{
	translate_sf_cereal(ar, qcr);
}

inline void serialize(Archive& ar, hxcomm::vx::quiggeldy_interface_types::response_type& qcr)
{
	translate_sf_cereal(ar, qcr);
}

// Note: Currently, hxcomm::vx::quiggeldy_interface_types::reinit_type is the
// same as the request_type and thus no additional declaration of serialize()
// is needed.

} // namespace SF

namespace hxcomm::vx {

RRWR_GENERATE_INTERFACE_EXPLICIT_TYPES(
    I_HXCommQuiggeldyVX,
    typename quiggeldy_interface_types::response_type,
    typename quiggeldy_interface_types::request_type,
    typename quiggeldy_interface_types::reinit_type)
RCF_METHOD_R1(std::string, get_unique_identifier, std::optional<std::string>)
RCF_METHOD_R0(std::string, get_version_string)
RCF_END(I_HXCommQuiggeldyVX)

namespace detail {

using rcf_client_type = hxcomm::vx::RcfClient<hxcomm::vx::I_HXCommQuiggeldyVX>;

} // namespace detail

} // namespace hxcomm::vx
