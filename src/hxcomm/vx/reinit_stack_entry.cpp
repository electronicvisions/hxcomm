#include "hxcomm/vx/reinit_stack_entry.h"
#include "hxcomm/common/reinit_stack_entry_impl.tcc"
#include "hxcomm/vx/connection_variant.h"

namespace hxcomm {

template class ReinitStackEntry<hxcomm::vx::QuiggeldyConnection, hxcomm::vx::ConnectionVariant>;

} // namespace hxcomm
