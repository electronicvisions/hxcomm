#pragma once
#include "hxcomm/common/quiggeldy_interface_types.h"
#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>

namespace cereal {

template <typename Archive, typename ConnectionParameter>
void CEREAL_SERIALIZE_FUNCTION_NAME(
    Archive& ar, typename hxcomm::detail::ReinitEntryType<ConnectionParameter>& reinit_entry)
{
	ar(CEREAL_NVP(reinit_entry.request));
	ar(CEREAL_NVP(reinit_entry.snapshot));
	ar(CEREAL_NVP(reinit_entry.reinit_pending));
}

} // namespace cereal
