#include "halco/hicann-dls/vx/jtag.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/vx/axiconnection.h"
#include "hxcomm/vx/utmessage.h"
#include <iostream>

using namespace hxcomm::vx;

using namespace hxcomm::vx::instruction;
using Data = instruction::to_fpga_jtag::Data;
using Ins = instruction::to_fpga_jtag::Ins;

using JTAGPhyRegisterOnDLS = halco::hicann_dls::vx::JTAGPhyRegisterOnDLS;

/**
 * Example routine to directly acces the TUD PHY
 */
template <typename Stream>
void phy_debug(Stream& stream, JTAGPhyRegisterOnDLS const& coord)
{
	// Reset sequence
	stream.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(true)));
	stream.add(UTMessageToFPGA<timing::Setup>());
	stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10)));
	stream.add(UTMessageToFPGA<system::Reset>(system::Reset::Payload(false)));
	stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(100)));
	stream.add(UTMessageToFPGA<to_fpga_jtag::Init>());

	// Set PLL
	stream.add(UTMessageToFPGA<Ins>(Ins::PLL_TARGET_REG));
	stream.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(3), 1)));
	stream.add(UTMessageToFPGA<Ins>(Ins::SHIFT_PLL));
	stream.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0xC0C3F200)));
	stream.add(UTMessageToFPGA<Ins>(Ins::PLL_TARGET_REG));
	stream.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(3), 3)));
	stream.add(UTMessageToFPGA<Ins>(Ins::SHIFT_PLL));
	stream.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0xC0C3F200)));


	// Write 8 bit TX data
	uint8_t const data = 0x12;
	stream.add(UTMessageToFPGA<Ins>(Ins::Payload((1 << 6) | (coord.toEnum() << 3) | 1)));
	stream.add(UTMessageToFPGA<Data>(
	    Data::Payload(/*keep_response*/ false, Data::Payload::NumBits(8), data)));

	// Read 8 bit RX data
	stream.add(UTMessageToFPGA<Ins>(Ins::Payload((1 << 6) | (coord.toEnum() << 3) | 0)));

	// Read 22 bit phy ctrl status
	stream.add(UTMessageToFPGA<Ins>(Ins::Payload((1 << 6) | (coord.toEnum() << 3) | 2)));


	// Halt execution
	stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10000)));
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	stream.run_until_halt();

	for (auto const message : stream.receive_all()) {
		std::visit([&message](auto m) { std::cout << m << std::endl; }, message);
	}
}


int main(int /*argc*/, char** /*argv[]*/)
{
	AXIConnection connection;
	auto stream = hxcomm::Stream(connection);
	phy_debug(stream, JTAGPhyRegisterOnDLS(0));
}
