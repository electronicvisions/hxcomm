#include "hxcomm/common/reinit_stack_entry.h"
#include "hxcomm/common/stream_rc.h"

namespace hxcomm {

template <typename QuiggeldyConnection, typename ConnectionVariant>
template <typename Connection>
ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::ReinitStackEntry(Connection& connection) :
    m_logger(log4cxx::Logger::getLogger("ReinitStackEntry")),
    m_connection_supports_reinit(supports_reinit_v<Connection>),
    m_connection_ref{std::ref(connection)}
{
	setup(connection);
}

template <typename QuiggeldyConnection, typename ConnectionVariant>
template <typename Connection>
void ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::setup(Connection& connection)
{
	if constexpr (supports_reinit_v<Connection>) {
		// preserve constness by declaring stream const even though we have to remove const-ness
		// for construction -> unfortunately there is no custom constructor for const
		StreamRC<quiggeldy_connection_type> const stream{connection};
		m_reinit_uploader = stream.get_reinit_upload();
		m_reinit_stack = stream.get_reinit_stack();
		m_idx_in_stack = std::make_optional(m_reinit_stack.lock()->push(reinit_entry_type{}));
	}
}

} // namespace hxcomm
