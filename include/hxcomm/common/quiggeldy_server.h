#pragma once

#include "hxcomm/common/quiggeldy_worker.h"

#include "rcf-extensions/round-robin-reinit-scheduler.h"

#ifdef USE_MUNGE_AUTH
#include <munge.h>
#endif

#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace log4cxx {

class Logger;

} // namespace log4cxx

namespace hxcomm {

template <typename Connection>
class QuiggeldyServer
    : public rcf_extensions::RoundRobinReinitScheduler<QuiggeldyWorker<Connection>>
{
public:
	using parent_t = rcf_extensions::RoundRobinReinitScheduler<QuiggeldyWorker<Connection>>;
	using parent_t::RoundRobinReinitScheduler;

	/**
	 * Bind QuiggeldyServer to the RcfInterface.
	 *
	 * Cannot be called on baseclass because then the baseclass would get bound
	 * to the RCF-interface.
	 */
	template <typename RcfInterface>
	void bind_to_interface()
	{
		parent_t::m_server->template bind<RcfInterface>(*this);
	}

	std::string get_unique_identifier(std::optional<std::string> hwdb_path)
	{
		return parent_t::visit_worker_const(
		    [&hwdb_path](auto const& worker) { return worker.get_unique_identifier(hwdb_path); });
	}
};

} // namespace hxcomm
