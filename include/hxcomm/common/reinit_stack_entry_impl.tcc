
#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/reinit_stack_entry.h"
#include "hxcomm/common/stream_rc.h"
#include <limits>
#include <sstream>

namespace hxcomm {

template <typename QuiggeldyConnection, typename ConnectionVariant>
ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::ReinitStackEntry(
    ConnectionVariant& variant) :
    m_logger(log4cxx::Logger::getLogger("ReinitStackEntry")),
    m_connection_supports_reinit{std::visit(
        [](auto& conn) { return supports_reinit_v<std::remove_cvref_t<decltype(conn)>>; },
        variant)},
    m_connection_ref{
        std::visit([](auto& conn) { return connection_ref_variant_type{std::ref(conn)}; }, variant)}
{
	std::visit([this](auto& conn) { setup(conn); }, variant);
}

template <typename QuiggeldyConnection, typename ConnectionVariant>
ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::~ReinitStackEntry()
{
	pop();
}

template <typename QuiggeldyConnection, typename ConnectionVariant>
void ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::pop()
{
	if (!m_connection_supports_reinit) {
		/* Nothing to do without reinit. */
	} else if (!m_idx_in_stack) {
		HXCOMM_LOG_TRACE(m_logger, "Entry already popped from stack.");
	} else {
		if (auto stack = m_reinit_stack.lock()) {
			try {
				stack->pop_entry(*m_idx_in_stack);
			} catch (std::exception& e) {
				HXCOMM_LOG_ERROR(m_logger, e.what());
				// In case of an error, we replace the removed entry with an empy pbmem
				stack->update_at(*m_idx_in_stack, reinit_entry_type{});
			}
			m_idx_in_stack.reset();
		} else {
			// Runtime check because ReinitStackEntry is exposed via Python bindings
			// and has to be valid for all connections.
			HXCOMM_LOG_DEBUG(m_logger, "Cannot pop reinit entry from the stack: Stack deleted.");
		}
	}
}

template <typename QuiggeldyConnection, typename ConnectionVariant>
void ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::set(reinit_entry_type&& entry)
{
	if (!m_connection_supports_reinit) {
		handle_unsupported_connection(entry);
	} else if (!m_idx_in_stack) {
		throw std::runtime_error("Trying to update an already popped reinit stack entry value.");
	} else if (auto uploader = m_reinit_uploader.lock()) {
		auto stack = m_reinit_stack.lock();
		if (!stack->update_at(*m_idx_in_stack, std::move(entry))) {
			throw std::runtime_error("Could not update reinit program.");
		}
#ifndef __GENPYBIND__ // unfortunately RCF-clients etc are undefined in genpybind-run
		stack->upload(*uploader);
#else
		std::ignore = entry;
#endif
	} else {
		// Runtime check because ReinitStackEntry is exposed via Python bindings
		// and has to be valid for all connections.
		HXCOMM_LOG_ERROR(m_logger, "Cannot register new reinit program: Uploader deleted.");
		throw std::runtime_error("Cannot register new reinit program: Uploader deleted.");
	}
}

template <typename QuiggeldyConnection, typename ConnectionVariant>
void ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::set(reinit_entry_type const& entry)
{
	// Runtime check because ReinitStackEntry is exposed via Python bindings
	// and has to be valid for all connections.
	if (!m_connection_supports_reinit) {
		handle_unsupported_connection(entry);
	} else if (auto uploader = m_reinit_uploader.lock()) {
		auto stack = m_reinit_stack.lock();
		if (!stack->update_at(*m_idx_in_stack, entry)) {
			throw std::runtime_error("Could not update reinit program.");
		}
#ifndef __GENPYBIND__ // unfortunately RCF-clients etc are undefined in genpybind-run
		stack->upload(*uploader);
#else
		std::ignore = entry;
#endif
	} else {
		HXCOMM_LOG_ERROR(m_logger, "Cannot register new reinit program: Uploader deleted.");
		throw std::runtime_error("Cannot register new reinit program: Uploader deleted.");
	}
}

template <typename QuiggeldyConnection, typename ConnectionVariant>
void ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::enforce()
{
	if (m_connection_supports_reinit) {
		StreamRC<quiggeldy_connection_type> stream(
		    std::get<std::reference_wrapper<quiggeldy_connection_type>>(m_connection_ref).get());
		stream.reinit_enforce();
	}
}

template <typename QuiggeldyConnection, typename ConnectionVariant>
void ReinitStackEntry<QuiggeldyConnection, ConnectionVariant>::handle_unsupported_connection(
    reinit_entry_type const& entry)
{
	if (entry.reinit_pending) {
		HXCOMM_LOG_TRACE(
		    m_logger, "Connection does not support upload of reinit program, treating enforced "
		              "reinit-like regular program to execute.");
		std::visit(
		    [&entry](auto& conn) { execute_messages(conn.get(), entry.request, true); },
		    m_connection_ref);
	} else {
		HXCOMM_LOG_TRACE(
		    m_logger, "Connection does not support upload of reinit program, ignoring "
		              "unenforced reinit program.");
	}
}

} // namespace hxcomm
