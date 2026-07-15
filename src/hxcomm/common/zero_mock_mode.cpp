#include "hxcomm/common/zero_mock_mode.h"
#include <cstdlib>

namespace hxcomm {

bool get_enable_zero_mock_mode()
{
	char const* enable_zero_mock = std::getenv("HXCOMM_ENABLE_ZERO_MOCK");
	if (enable_zero_mock && atoi(enable_zero_mock)) {
		return true;
	}
	return false;
}

} // namespace hxcomm
