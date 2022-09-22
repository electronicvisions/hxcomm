#pragma once

#include "hxcomm/common/quiggeldy_interface_types.h"
#include <memory>
#include <thread>

namespace hxcomm {

/**
 * ReinitStack holds the stack of reinit programs that are currently set as a
 * simple wrapper around a vector.
 *
 * Since ReinitStack resides within the QuiggeldyConnection-handle, ReinitStack
 * needs to be constructable prior to the full definition of QuiggeldyConnection.
 * This is why informing the remote side about changes in the stack is handled
 * by ReinitStackEntry.
 */
template <typename ConnectionParameters>
class ReinitStack
{
public:
	using interface_types = quiggeldy_interface_types<ConnectionParameters>;
	using reinit_type = typename interface_types::reinit_type;
	using reinit_entry_type = typename interface_types::reinit_entry_type;

	ReinitStack();
	ReinitStack(ReinitStack&&) = default;
	ReinitStack(ReinitStack const&) = delete;

	/**
	 * Pop entry with given index.
	 *
	 * Throws if the given index is not the topmost entry in the stack.
	 *
	 * @param idx Index to delete.
	 */
	void pop_entry(std::size_t idx);

	/**
	 * Push a new entry onto the stack and return its position within the stack.
	 *
	 * @param entry The entry to be pushed on the stack
	 * @return Position at which entry resides in stack after pushing.
	 */
	std::size_t push(reinit_entry_type&& entry);

	/**
	 * Push a new entry onto the stack and return its position within the stack.
	 *
	 * @param entry The entry to be pushed on the stack
	 * @return Position at which entry resides in stack after pushing.
	 */
	std::size_t push(reinit_entry_type const& entry);

	/**
	 * Update entry at given position.
	 *
	 * @param idx Position at which to update the entry.
	 * @param entry New updated entry in the stack.
	 * @return if entry was succesfully updated
	 */
	bool update_at(std::size_t idx, reinit_entry_type&& entry);

	/**
	 * Update entry at given position.
	 *
	 * @param idx Position at which to update the entry.
	 * @param entry New updated entry in the stack.
	 * @return if entry was succesfully updated
	 */
	bool update_at(std::size_t idx, reinit_entry_type const& entry);

	/**
	 * Register the current reinit stack for upload with the provided uploader.
	 *
	 * @tparam UploaderT The uploader to register the current stack to.
	 */
	template <typename UploaderT>
	void upload(UploaderT&) const;

private:
	log4cxx::LoggerPtr m_logger;

	mutable std::mutex m_mutex;
	reinit_type m_stack;
};

} // namespace hxcomm

#include "hxcomm/common/reinit_stack.tcc"
