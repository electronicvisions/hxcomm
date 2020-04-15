
#include "hxcomm/common/cerealization_utmessage.h"
#include "hxcomm/common/sf_serialization.h"

#include "SF/string.hpp"
#include "SF/variant.hpp"
#include "SF/vector.hpp"
#include "cereal/types/boost_variant.hpp"
#include "cereal/types/vector.hpp"

namespace SF {

// explicitly instantiate needed serialization
void serialize(SF::Archive& ar, QUIGGELDY_REQUEST_T& qcr)
{
	translate_sf_cereal(ar, qcr);
}

void serialize(SF::Archive& ar, QUIGGELDY_RESPONSE_T& qcr)
{
	translate_sf_cereal(ar, qcr);
}

} // namespace SF
