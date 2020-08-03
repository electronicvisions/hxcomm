
#include "hxcomm/common/register_reinit.h"

#include "hxcomm/common/stream.h"

#ifdef __GENPYBIND__ // unfortunately RCF-clients etc are undefined in genpybind-run
#include <tuple>
#endif


namespace hxcomm {

template <typename QuiggeldyConnection>
template <typename Connection>
RegisterReinit<QuiggeldyConnection>::RegisterReinit(Connection const& connection) :
    m_log(log4cxx::Logger::getLogger("RegisterReinit")),
    m_connection_supports_reinit(supports_reinit_v<Connection>)
{
	setup_uploader(connection);
}

template <typename QuiggeldyConnection>
void RegisterReinit<QuiggeldyConnection>::operator()(reinit_type&& reinit)
{
	if (m_connection_supports_reinit) {
		if (auto uploader = m_uploader_reinit.lock()) {
#ifndef __GENPYBIND__ // unfortunately RCF-clients etc are undefined in genpybind-run
			uploader->upload(std::move(reinit));
#else
			std::ignore = reinit;
#endif
		} else {
			HXCOMM_LOG_ERROR(m_log, "Cannot register new reinit program: Uploader deleted.");
			throw std::runtime_error("Cannot register new reinit program: Uploader deleted.");
		}
	} else {
		HXCOMM_LOG_TRACE(m_log, "Connection does not support upload of reinit program, ignoring.");
	}
}

template <typename QuiggeldyConnection>
void RegisterReinit<QuiggeldyConnection>::operator()(reinit_type const& reinit)
{
	if (m_connection_supports_reinit) {
		if (auto uploader = m_uploader_reinit.lock()) {
#ifndef __GENPYBIND__ // unfortunately RCF-clients etc are undefined in genpybind-run
			uploader->upload(reinit);
#else
			std::ignore = reinit;
#endif
		} else {
			HXCOMM_LOG_ERROR(m_log, "Cannot register new reinit program: Uploader deleted.");
			throw std::runtime_error("Cannot register new reinit program: Uploader deleted.");
		}
	} else {
		HXCOMM_LOG_TRACE(m_log, "Connection does not support upload of reinit program, ignoring.");
	}
}

} // namespace hxcomm
