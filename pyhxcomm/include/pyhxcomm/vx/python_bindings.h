#include "pyhxcomm/common/managed_connection.h"

// connections need to go first, otherwise the bindings will not have the
// alias-names
#ifdef WITH_HOSTARQ
#include "pyhxcomm/vx/arqconnection.h"
#endif
#ifdef WITH_EXTOLL
#include "pyhxcomm/vx/extollconnection.h"
#endif
#include "pyhxcomm/vx/quiggeldy_connection.h"
#include "pyhxcomm/vx/simconnection.h"
#include "pyhxcomm/vx/zeromockconnection.h"

#include "pyhxcomm/vx/connection_handle_binding.h"
#include "pyhxcomm/vx/quiggeldy_utility.h"
#include "pyhxcomm/vx/reinit_stack_entry.h"

#include "pyhxcomm/vx/connection_handle_binding.h"
#include "pyhxcomm/vx/connection_time_info.h"
#include "pyhxcomm/vx/get_repo_state.h"
