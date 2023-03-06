
#include "hxcomm/common/reinit_stack.h"

#include <sstream>

namespace hxcomm {

template <typename CP>
ReinitStack<CP>::ReinitStack() : m_logger(log4cxx::Logger::getLogger("ReinitStack"))
{}

template <typename CP>
void ReinitStack<CP>::pop_entry(std::size_t idx)
{
	std::lock_guard lk{m_mutex};

	if (m_stack.size() == 0) {
		auto const msg = "Trying to pop from empty reinit stack.";

		HXCOMM_LOG_ERROR(m_logger, msg);
		throw std::runtime_error(msg);
	}

	auto topmost = m_stack.size() - 1;
	if (idx != topmost) {
		std::stringstream ss;
		ss << "Trying to pop reinit entry #" << idx << " from reinit stack but #" << topmost
		   << " is on top. Aborting..";
		auto const msg = ss.str();
		HXCOMM_LOG_ERROR(m_logger, msg);
		throw std::runtime_error(msg);
	} else {
		m_stack.pop_back();
	}
}

template <typename CP>
std::size_t ReinitStack<CP>::push(reinit_entry_type&& entry)
{
	std::lock_guard lk{m_mutex};
	auto const size = m_stack.size();
	m_stack.push_back(std::move(entry));
	return size;
}

template <typename CP>
std::size_t ReinitStack<CP>::push(reinit_entry_type const& entry)
{
	std::lock_guard lk{m_mutex};
	auto const size = m_stack.size();
	m_stack.push_back(entry);
	return size;
}

template <typename CP>
bool ReinitStack<CP>::update_at(std::size_t idx, reinit_entry_type&& entry)
{
	std::lock_guard lk{m_mutex};
	if (idx >= m_stack.size()) {
		HXCOMM_LOG_ERROR(m_logger, "Updating invalid entry in reinit stack.");
		return false;
	}
	m_stack[idx] = std::move(entry);
	return true;
}

template <typename CP>
bool ReinitStack<CP>::update_at(std::size_t idx, reinit_entry_type const& entry)
{
	std::lock_guard lk{m_mutex};
	if (idx >= m_stack.size()) {
		HXCOMM_LOG_ERROR(m_logger, "Updating invalid entry in reinit stack.");
		return false;
	}
	m_stack[idx] = entry;
	return true;
}

template <typename CP>
template <typename UploaderT>
void ReinitStack<CP>::upload(UploaderT& uploader) const
{
	std::lock_guard lk{m_mutex};
	reinit_type const& cref{m_stack};
	uploader.upload(cref);
}

template <typename CP>
void ReinitStack<CP>::set_all_done()
{
	std::lock_guard lk{m_mutex};
	for (auto& entry : m_stack) {
		entry.reinit_pending = false;
	}
}

} // namespace hxcomm
