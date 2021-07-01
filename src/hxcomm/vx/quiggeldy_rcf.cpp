#include "hxcomm/vx/quiggeldy_rcf.h"

#include "cereal/types/utility.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/types/vector.hpp"

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

namespace SF {

void serialize(Archive& ar, hxcomm::vx::quiggeldy_interface_types::request_type& qcr)
{
	translate_sf_cereal(ar, qcr);
}

void serialize(Archive& ar, hxcomm::vx::quiggeldy_interface_types::response_type& qcr)
{
	translate_sf_cereal(ar, qcr);
}

// Note: Currently, hxcomm::vx::quiggeldy_interface_types::reinit_type is the
// same as the request_type and thus no additional declaration of serialize()
// is needed.

} // namespace SF
