#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/hwdb_entry.h"
#include "hxcomm/common/sf_serialization.h"

namespace SF {

void serialize(Archive& ar, hxcomm::HwdbEntry& value) SYMBOL_VISIBLE;

} // namespace SF
