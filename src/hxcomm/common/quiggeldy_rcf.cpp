#include "hxcomm/common/quiggeldy_rcf.h"

#include "cereal/types/hxcomm/common/hwdb_entry.h"
#include "hxcomm/common/hwdb_entry.h"
#include <cereal/types/variant.hpp>

namespace SF {

void serialize(Archive& ar, hxcomm::HwdbEntry& value)
{
	translate_sf_cereal(ar, value);
}

} // namespace SF
