#pragma once
#include "cereal/types/halco/common/geometry.h"
#include "cereal/types/halco/common/misc_types.h"
#include "hwdb4cpp/hwdb4cpp.h"
#include "hxcomm/common/hwdb_entry.h"
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>

namespace cereal {

template <typename Archive>
void serialize(Archive& ar, hwdb4cpp::HXCubeWingEntry& value)
{
	ar(CEREAL_NVP(value.handwritten_chip_serial));
	ar(CEREAL_NVP(value.chip_revision));
	ar(CEREAL_NVP(value.eeprom_chip_serial));
	ar(CEREAL_NVP(value.synram_timing_pcconf));
	ar(CEREAL_NVP(value.synram_timing_wconf));
}

template <typename Archive>
void serialize(Archive& ar, hwdb4cpp::HXCubeFPGAEntry& value)
{
	ar(CEREAL_NVP(value.ip));
	ar(CEREAL_NVP(value.wing));
	ar(CEREAL_NVP(value.fuse_dna));
	ar(CEREAL_NVP(value.extoll_node_id));
	ar(CEREAL_NVP(value.ci_test_node));
}

template <typename Archive>
void serialize(Archive& ar, hwdb4cpp::HXCubeSetupEntry& value)
{
	ar(CEREAL_NVP(value.hxcube_id));
	ar(CEREAL_NVP(value.fpgas));
	ar(CEREAL_NVP(value.usb_host));
	ar(CEREAL_NVP(value.usb_serial));
	ar(CEREAL_NVP(value.xilinx_hw_server));
}

template <typename Archive>
void serialize(Archive& ar, hwdb4cpp::JboaSetupEntry& value)
{
	ar(CEREAL_NVP(value.jboa_id));
	ar(CEREAL_NVP(value.fpgas));
	ar(CEREAL_NVP(value.xilinx_hw_server));
}

template <typename Archive>
void serialize(Archive&, hxcomm::SimulationEntry&)
{
}

template <typename Archive>
void serialize(Archive&, hxcomm::ZeroMockEntry&)
{
}

} // namespace cereal
