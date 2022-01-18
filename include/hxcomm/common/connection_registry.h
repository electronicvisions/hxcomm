#pragma once
#include <mutex>
#include <set>
#include <thread>

namespace hxcomm {

/**
 * Registry of open connections.
 * @tparam Connection Connection type of registry
 */
template <typename Connection>
struct ConnectionRegistry
{
	typedef typename Connection::init_parameters_type Parameters;
	Parameters m_parameters;
	static std::set<Parameters>& registry();
	static std::mutex& registry_mutex();

	ConnectionRegistry(Parameters const& parameters);
	~ConnectionRegistry();
};

} // namespace hxcomm
