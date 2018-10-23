#pragma once

#include <cstdint>

namespace hxcomm {
class JTAG
{
public:
	/// Actual HICANN-X JTAG instructions according to chip specification
	enum instr : uint32_t
	{
		EXTEST = 0,
		IDCODE = 1,
		SAMPLE_PRELOAD = 2,
		PLL_TARGET_REG = 3,
		SHIFT_PLL = 4,
		OMNIBUS_ADDRESS = 5,
		OMNIBUS_DATA = 6,
		OMNIBUS_REQUEST = 7,
		BYPASS = 15
	};
};
} // namespace hxcomm
