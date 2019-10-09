#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stddef.h>

namespace hxcomm {

/**
 * Single producer single consumer double buffer access wrapper.
 * The producer waits on full buffer, the consumer waits on empty buffer.
 * Before a buffer access, start_{read,write} and after finishing access stop_{read,write} is to be
 * called.
 */
template <typename T>
class DoubleBuffer
{
public:
	/**
	 * Construct empty double buffer.
	 * @param run Atomic bool to check for validity on wait in start_{read,write}. If this value is
	 * false the functions return even if no {readable,writable} buffer is available.
	 */
	DoubleBuffer(std::atomic<bool>& run);

	/**
	 * Destruct double buffer.
	 */
	~DoubleBuffer();

	/**
	 * Start write access. Blocks on full buffer until stop_read marked an entry as writable.
	 * If m_run value is false return even if no writable buffer is available.
	 * @return Pointer to next writable buffer entry
	 */
	T* start_write();

	/**
	 * Stop write access. Sets next write position to other entry, atomically increases the
	 * number of readable entries and notifies the potentially waiting consumer.
	 */
	void stop_write();

	/**
	 * Start read access. Blocks on empty buffer until stop_write marked an entry as readable.
	 * If m_run value is false return even if no readable buffer is available.
	 * @return Constant pointer to next readable buffer entry
	 */
	T const* start_read();

	/**
	 * Stop read access. Sets next read position to other entry, atomically increases the number of
	 * writable entries and notifies the potentially waiting producer.
	 */
	void stop_read();

	/**
	 * Notify potentially waiting start_write and start_read calls.
	 */
	void notify();

private:
	T* m_data[2];

	std::atomic<size_t> m_read_available_count;

	size_t m_write_position;
	size_t m_read_position;

	std::mutex m_mutex;
	std::condition_variable m_cv;

	std::atomic<bool>& m_run;
};


/**
 * Packet consisting of array-like data and valid-count information.
 * The maximal number of valid entries is bound by the array size.
 * @tparam SubpacketType Type of entries
 * @tparam MaxSize Maximal number of valid packet entries
 */
template <typename SubpacketType, size_t MaxSize>
class Packet
{
public:
	typedef std::array<SubpacketType, MaxSize> array_type;

	/** Default construct packet. */
	Packet();

	/**
	 * Packet data. Valid portions are determined by the current size.
	 */
	array_type data;

	/**
	 * Set number of valid entries. Assertion on (size < MaxSize) is in place.
	 */
	void set_size(size_t const size);

	/**
	 * Get number of valid entries.
	 * @return Number
	 */
	size_t get_size() const;

	typename array_type::const_iterator cbegin() const;
	typename array_type::const_iterator cend() const;

private:
	size_t m_size;
};

} // namespace hxcomm

#include "hxcomm/common/double_buffer.tcc"
