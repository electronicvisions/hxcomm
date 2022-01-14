#pragma once
#include "hxcomm/vx/utmessage.h"
#include <iostream>

using namespace hxcomm::vx;

using namespace hxcomm::vx::instruction;
using Data = instruction::to_fpga_jtag::Data;
using Ins = instruction::to_fpga_jtag::Ins;

/**
 * Example routine to reset chip and read JTAG-ID.
 */
template <typename Stream>
void reset_and_id_readout(Stream& stream)
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

	// Read ID
	stream.add(UTMessageToFPGA<Ins>(Ins::IDCODE));
	stream.add(UTMessageToFPGA<Data>(Data::Payload(true, Data::Payload::NumBits(32), 0)));

	// Halt execution
	stream.add(UTMessageToFPGA<timing::WaitUntil>(timing::WaitUntil::Payload(10000)));
	stream.add(UTMessageToFPGA<system::Loopback>(system::Loopback::halt));

	stream.commit();

	stream.run_until_halt();

	for (auto const message : stream.receive_all()) {
		std::visit([&message](auto m) { std::cout << m << std::endl; }, message);
	}
}
