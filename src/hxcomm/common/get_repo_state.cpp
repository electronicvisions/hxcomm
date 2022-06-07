#include "hxcomm/common/get_repo_state.h"

#ifndef HXCOMM_REPO_STATE
#error "Needs HXCOMM_REPO_STATE for get_repo_state()"
#endif

namespace hxcomm {

std::string get_repo_state()
{
	return HXCOMM_REPO_STATE;
}

} // namespace hxcomm
