#include <typeindex>
#include <unordered_map>
#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/instruction.h"
#include "test-to_testing_types.h"

using namespace hxcomm::vx::instruction;

/**
 * Map of string representation of default-constructed payload.
 */
std::unordered_map<std::type_index, std::string> expected_string_representation = {
    {typeid(to_fpga_jtag::Init), "hxcomm::vx::instruction::to_fpga_jtag::Init()"},
    {typeid(to_fpga_jtag::Ins), "hxcomm::vx::instruction::to_fpga_jtag::Ins(0000000)"},
    {typeid(to_fpga_jtag::Scaler), "hxcomm::vx::instruction::to_fpga_jtag::Scaler(0)"},
    {typeid(to_fpga_jtag::Data),
     "hxcomm::vx::instruction::to_fpga_jtag::Data(keep_response: false, num_bits: 33, payload: "
     "000000000000000000000000000000000)"},
    {typeid(timing::Setup), "hxcomm::vx::instruction::timing::Setup()"},
    {typeid(timing::WaitUntil), "hxcomm::vx::instruction::timing::WaitUntil(0)"},
    {typeid(timing::SystimeInit), "hxcomm::vx::instruction::timing::SystimeInit(0)"},
    {typeid(timing::Barrier), "hxcomm::vx::instruction::timing::Barrier(000)"},
    {typeid(system::Loopback), "hxcomm::vx::instruction::system::Loopback(0)"},
    {typeid(system::Reset), "hxcomm::vx::instruction::system::Reset(0)"},
    {typeid(omnibus_to_fpga::Address), "hxcomm::vx::instruction::omnibus_to_fpga::Address(is_read: "
                                       "false, byte_enables: 1111, address: 0)"},
    {typeid(omnibus_to_fpga::Data), "hxcomm::vx::instruction::omnibus_to_fpga::Data(0)"},
    {typeid(event_to_fpga::SpikePack<1>),
     "hxcomm::vx::instruction::event_to_fpga::SpikePack<1ul>(0000000000000000)"},
    {typeid(event_to_fpga::SpikePack<2>),
     "hxcomm::vx::instruction::event_to_fpga::SpikePack<2ul>(0000000000000000, 0000000000000000)"},
    {typeid(event_to_fpga::SpikePack<3>), "hxcomm::vx::instruction::event_to_fpga::SpikePack<3ul>("
                                          "0000000000000000, 0000000000000000, 0000000000000000)"},
    {typeid(jtag_from_hicann::Data),
     "hxcomm::vx::instruction::jtag_from_hicann::Data(000000000000000000000000000000000)"},
    {typeid(omnibus_from_fpga::Data), "hxcomm::vx::instruction::omnibus_from_fpga::Data(0)"},
    {typeid(from_fpga_system::Loopback), "hxcomm::vx::instruction::from_fpga_system::Loopback(0)"},
    {typeid(timing_from_fpga::Sysdelta),
     "hxcomm::vx::instruction::timing_from_fpga::Sysdelta(00000000)"},
    {typeid(timing_from_fpga::Systime), "hxcomm::vx::instruction::timing_from_fpga::Systime("
                                        "0000000000000000000000000000000000000000000)"},
    {typeid(event_from_fpga::SpikePack<1>), "hxcomm::vx::instruction::event_from_fpga::SpikePack<"
                                            "1ul>(Spike(0000000000000000, 00000000))"},
    {typeid(event_from_fpga::SpikePack<2>),
     "hxcomm::vx::instruction::event_from_fpga::SpikePack<2ul>(Spike(0000000000000000, "
     "00000000), Spike(0000000000000000, 00000000))"},
    {typeid(event_from_fpga::SpikePack<3>),
     "hxcomm::vx::instruction::event_from_fpga::SpikePack<3ul>(Spike(0000000000000000, "
     "00000000), Spike(0000000000000000, 00000000), Spike(0000000000000000, "
     "00000000))"},
    {typeid(event_from_fpga::MADCSamplePack<1>),
     "hxcomm::vx::instruction::event_from_fpga::MADCSamplePack<1ul>(MADCSample(00000000000000, "
     "00000000))"},
    {typeid(event_from_fpga::MADCSamplePack<2>),
     "hxcomm::vx::instruction::event_from_fpga::MADCSamplePack<2ul>(MADCSample(00000000000000, "
     "00000000), MADCSample(00000000000000, 00000000))"},
    {typeid(event_from_fpga::MADCSamplePack<3>),
     "hxcomm::vx::instruction::event_from_fpga::MADCSamplePack<3ul>(MADCSample(00000000000000, "
     "00000000), MADCSample(00000000000000, 00000000), MADCSample(00000000000000, 00000000))"},
};

TEST(InstructionPayload, OstreamCoverage)
{
	constexpr size_t instruction_type_count = hate::type_list_size<ToFPGADictionary>::value +
	                                          hate::type_list_size<FromFPGADictionary>::value;

	EXPECT_EQ(expected_string_representation.size(), instruction_type_count);
}

template <class T>
class InstructionPayloadTests : public ::testing::Test
{};

typedef typename to_testing_types<ToFPGADictionary, FromFPGADictionary>::type UTMessageTypes;

TYPED_TEST_CASE(InstructionPayloadTests, UTMessageTypes);

TYPED_TEST(InstructionPayloadTests, Ostream)
{
	typedef typename TypeParam::instruction_type InstructionType;

	typename InstructionType::Payload payload;
	std::stringstream actual;
	actual << payload;

	// coverage check is done above
	if (expected_string_representation.find(typeid(InstructionType)) !=
	    expected_string_representation.end()) {
		EXPECT_EQ(expected_string_representation.at(typeid(InstructionType)), actual.str());
	}
}
