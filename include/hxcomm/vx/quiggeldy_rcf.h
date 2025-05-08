#pragma once

#include "cereal/types/utility.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/types/vector.hpp"

#include "cereal/types/hxcomm/common/quiggeldy_interface_types.h"
#include "cereal/types/hxcomm/common/utmessage.h"
#include "hate/visibility.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/hwdb_entry.h"
#include "hxcomm/common/quiggeldy_rcf.h"
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

void serialize(Archive& ar, hxcomm::vx::quiggeldy_interface_types::request_type& qcr)
    SYMBOL_VISIBLE;

void serialize(Archive& ar, hxcomm::vx::quiggeldy_interface_types::response_type& qcr)
    SYMBOL_VISIBLE;

void serialize(Archive& ar, hxcomm::vx::quiggeldy_interface_types::reinit_entry_type& qcr)
    SYMBOL_VISIBLE;

} // namespace SF

namespace hxcomm::vx {

RRWR_GENERATE_INTERFACE_EXPLICIT_TYPES(
    I_HXCommQuiggeldyVX,
    typename quiggeldy_interface_types::response_type,
    typename quiggeldy_interface_types::request_type,
    typename quiggeldy_interface_types::reinit_type)
RCF_METHOD_R1(std::string, get_unique_identifier, std::optional<std::string>)
RCF_METHOD_R0(std::string, get_bitfile_info)
RCF_METHOD_R0(std::string, get_remote_repo_state)
RCF_METHOD_R0(std::string, get_version_string)
RCF_METHOD_R0(hxcomm::HwdbEntry, get_hwdb_entry)
RCF_METHOD_R0(bool, get_use_munge)
RCF_END(I_HXCommQuiggeldyVX)

namespace detail {

using rcf_client_type = hxcomm::vx::RcfClient<hxcomm::vx::I_HXCommQuiggeldyVX>;

} // namespace detail

} // namespace hxcomm::vx
