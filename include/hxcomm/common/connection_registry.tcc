#include "hxcomm/common/connection_registry.h"

#include <stdexcept>

namespace hxcomm {

template <typename Connection>
ConnectionRegistry<Connection>::ConnectionRegistry(Parameters const& parameters) :
    m_parameters(parameters)
{
	std::lock_guard lock(registry_mutex());
	if (registry().count(parameters)) {
		throw std::runtime_error("Trying to construct Connection to already connected endpoint.");
	}
	registry().insert(parameters);
}

template <typename Connection>
std::set<typename ConnectionRegistry<Connection>::Parameters>&
ConnectionRegistry<Connection>::registry()
{
	static std::set<Parameters>* storage = nullptr;
	if (!storage) {
		storage = new std::set<Parameters>();
	}
	assert(storage);
	return *storage;
}

template <typename Connection>
std::mutex& ConnectionRegistry<Connection>::registry_mutex()
{
	static std::mutex storage;
	return storage;
}

template <typename Connection>
ConnectionRegistry<Connection>::~ConnectionRegistry()
{
	std::lock_guard lock(registry_mutex());
	if (registry().count(m_parameters)) {
		registry().erase(m_parameters);
	}
}

} // namespace hxcomm
