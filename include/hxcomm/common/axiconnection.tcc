#include <boost/asio/ip/host_name.hpp>
#include "hate/math.h"
#include "hate/timer.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/signal.h"

#include <chrono>
#include <cstdint>
#include <cstring>

extern "C" {
#include <sys/mman.h>
#include <unistd.h>
}

namespace hxcomm {

struct AXIHandle
{
	typedef uint64_t word_type;

	/** Create AXIHandle.
	 * Opens /dev/mem and tries to mmap RX/TX addresses.
	 */
	inline AXIHandle(off_t base_reset, off_t base_tx_data, off_t base_tx_regs, off_t base_rx_data, off_t base_rx_regs);

	/** Destructs AXIHandle.
	 * Unmmaps memory regions and closes /dev/mem fd.
	 */
	inline ~AXIHandle() noexcept(false);

	/** Send payload words via AXI.
	 */
	inline void send(std::queue<uint64_t>&);

	/** Receive payload words from AXI.
	 */
	inline std::vector<word_type> receive();

	/** Trigger release of program.
	 */
	inline void trigger_unhold();

private:
	log4cxx::Logger* m_logger;

	// file descript pointing to /dev/mem
	int m_fd;

	// page size (needed for mmap alignment purposes)
	long m_map_size;

	void* m_mem_reset = nullptr;
	off_t const m_base_reset; // 0xa000'0000

	void* m_mem_tx_data = nullptr;
	off_t const m_base_tx_data; // 0xa001'0000
	off_t static const m_woff_tx_data_lo = 0x0; // write-only
	off_t static const m_woff_tx_data_hi = 0x8; // write-only

	void* m_mem_tx_regs = nullptr;
	off_t const m_base_tx_regs; // 0xa002'0000
	off_t static const m_woff_tx_control = 0x0; // write-only
	off_t static const m_woff_tx_status = 0x8; // read-only
	off_t static const m_boff_tx_status_ready = 31; // FIFO ready
	off_t static const m_mask_tx_status_count_mask = 0x7FFFFFFF; // write count

	void* m_mem_rx_data = nullptr;
	off_t const m_base_rx_data; // 0xa003'0000
	off_t static const m_woff_rx_data_lo = 0x0;
	off_t static const m_woff_rx_data_hi = 0x8;

	void* m_mem_rx_regs = nullptr;
	off_t const m_base_rx_regs; // 0xa004'0000
	off_t static const m_woff_rx_control = 0x0;
	off_t static const m_woff_rx_status = 0x8;
	off_t static const m_boff_rx_status_ready = 31; // FIFO valid
	off_t static const m_mask_rx_status_count_mask = 0x7FFFFFFF; // read count

	union TxStatus {
		uint32_t raw;
		struct __attribute__((packed)) {
			uint32_t count : 31; // write count
			uint32_t ready :  1; // ready bit
		} bit;
	};

	TxStatus read_tx_status() {
		TxStatus ret(*(volatile uint32_t*)((char*)m_mem_tx_regs + m_woff_tx_status));
		return ret;
	}

	union TxControl {
		uint32_t raw;
		struct __attribute__((packed)) {
			uint32_t push :  1; // push data
			uint32_t hold :  1; // hold back playback
			uint32_t      : 30; // padding
		} bit;
	};

	void write_tx_control(TxControl const txc) {
		*(volatile uint32_t*)((char*)m_mem_tx_regs + m_woff_tx_control) = txc.raw;
	}

	void write_tx_data(uint64_t const& value) {
		*(volatile uint32_t*)((char*)m_mem_tx_data + m_woff_tx_data_lo) = static_cast<uint32_t>(value);
		*(volatile uint32_t*)((char*)m_mem_tx_data + m_woff_tx_data_hi) = static_cast<uint32_t>(value >> 32);
	}

	union RxStatus {
		uint32_t raw;
		struct __attribute__((packed)) {
			uint32_t count : 31; // read count
			uint32_t valid :  1; // valid bit
		} bit;
	};

	RxStatus read_rx_status() {
		RxStatus ret(*(volatile uint32_t*)((char*)m_mem_rx_regs + m_woff_rx_status));
		return ret;
	}

	union RxControl {
		uint32_t raw;
		struct __attribute__((packed)) {
			uint32_t push :  1; // push data
			uint32_t      : 31; // padding
		} bit;
	};

	void write_rx_control(RxControl const rxc) {
		*(volatile uint32_t*)((char*)m_mem_rx_regs + m_woff_rx_control) = rxc.raw;
	}

	uint64_t read_rx_data() {
		uint32_t lo = *(volatile uint32_t*)((char*)m_mem_rx_data + m_woff_rx_data_lo);
		uint32_t hi = *(volatile uint32_t*)((char*)m_mem_rx_data + m_woff_rx_data_hi);
		uint64_t ret = static_cast<uint64_t>(lo) | (static_cast<uint64_t>(hi) << 32);
		return ret;
	}
};

AXIHandle::AXIHandle(
	off_t base_reset = 0xa000'0000,
	off_t base_tx_data = 0xa001'0000,
	off_t base_tx_regs = 0xa002'0000,
	off_t base_rx_data = 0xa003'0000,
	off_t base_rx_regs = 0xa004'0000) :
	m_logger(log4cxx::Logger::getLogger("hxcomm.AXIHandle")),
	m_base_reset(base_reset),
	m_base_tx_data(base_tx_data),
	m_base_tx_regs(base_tx_regs),
	m_base_rx_data(base_rx_data),
	m_base_rx_regs(base_rx_regs)
{
	m_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (m_fd == -1) {
		std::stringstream ss;
		ss << "Cannot open /dev/mem: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	// need two pages due to base addresses being 4096 apart...
	m_map_size = 1 * sysconf(_SC_PAGE_SIZE);
	if (m_map_size == -1) {
		std::stringstream ss;
		ss << "Cannot acquire page size: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
	off_t const map_mask = m_map_size - 1;

	// express some assumptions
	assert(m_map_size == 1 * 4096); // size of pages

	// map in single pages
	m_mem_reset = mmap(0, m_map_size,              PROT_WRITE , MAP_SHARED_VALIDATE, m_fd, m_base_reset & ~map_mask);
	if (m_mem_reset == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap register region: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
	m_mem_tx_data = mmap(0, m_map_size,              PROT_WRITE , MAP_SHARED_VALIDATE, m_fd, m_base_tx_data & ~map_mask);
	if (m_mem_tx_data == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap register region: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
	m_mem_tx_regs = mmap(0, m_map_size, (PROT_READ | PROT_WRITE), MAP_SHARED_VALIDATE, m_fd, m_base_tx_regs & ~map_mask);
	if (m_mem_tx_regs == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap register region: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
	m_mem_rx_data = mmap(0, m_map_size,  PROT_READ              , MAP_SHARED_VALIDATE, m_fd, m_base_rx_data & ~map_mask);
	if (m_mem_rx_data == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap register region: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
	m_mem_rx_regs = mmap(0, m_map_size, (PROT_READ | PROT_WRITE), MAP_SHARED_VALIDATE, m_fd, m_base_rx_regs & ~map_mask);
	if (m_mem_rx_regs == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap register region: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	// reset synplify wrapper
	*(volatile uint32_t*)((char*)m_mem_reset) = 2;
	*(volatile uint32_t*)((char*)m_mem_reset) = 0;
}

AXIHandle::~AXIHandle() noexcept(false)
{
	// exceptions trigger call to std::terminate

	if (m_mem_reset && munmap(m_mem_reset, m_map_size) == -1) {
		std::stringstream ss;
		ss << "Cannot unmap memory: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	if (m_mem_tx_data && munmap(m_mem_tx_data, m_map_size) == -1) {
		std::stringstream ss;
		ss << "Cannot unmap memory: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	if (m_mem_tx_regs && munmap(m_mem_tx_regs, m_map_size) == -1) {
		std::stringstream ss;
		ss << "Cannot unmap memory: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	if (m_mem_rx_data && munmap(m_mem_rx_data, m_map_size) == -1) {
		std::stringstream ss;
		ss << "Cannot unmap memory: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	if (m_mem_rx_regs && munmap(m_mem_rx_regs, m_map_size) == -1) {
		std::stringstream ss;
		ss << "Cannot unmap memory: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	if (close(m_fd) == -1) {
		std::stringstream ss;
		ss << "Cannot close file descriptor: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
}

void AXIHandle::send(std::queue<uint64_t>& q)
{
	while (!q.empty()) {
		TxStatus status;
		do {
			status = read_tx_status();
		} while (!status.bit.ready);

		write_tx_data(q.front());
		q.pop();

		TxControl txc{0};
		txc.bit.push = 1;
		txc.bit.hold = 1;
		write_tx_control(txc);
		txc.bit.push = 0;
		write_tx_control(txc);
	}
}

std::vector<AXIHandle::word_type> AXIHandle::receive()
{
	std::vector<AXIHandle::word_type> ret;

	RxStatus status;
	while (true) {
		status = read_rx_status();
		if (!status.bit.valid) {
			return ret;
		}

		ret.push_back(read_rx_data());

		write_rx_control({1});
		write_rx_control({0});
	}

	return ret;
}
	
void AXIHandle::trigger_unhold()
{
	TxControl txc{0};
	txc.bit.push = 0;
	txc.bit.hold = 0;
	write_tx_control(txc);
}


template <typename ConnectionParameter>
AXIConnection<ConnectionParameter>::AXIConnection() :
    m_axi(std::make_unique<AXIHandle>()),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.AXIConnection")),
    m_worker_receive(&AXIConnection<ConnectionParameter>::work_receive, this)
{}

template <typename ConnectionParameter>
AXIConnection<ConnectionParameter>::AXIConnection(AXIConnection&& other) :
	m_axi(),
    m_send_queue(other.m_send_queue),
    m_encoder(other.m_encoder, m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt), // temporary
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.AXIConnection")),
    m_worker_receive()
{
	// shutdown other threads
	other.m_run_receive = false;
	other.m_worker_receive.join();
	m_encode_duration = other.m_encode_duration.load(std::memory_order_relaxed);
	m_decode_duration = other.m_decode_duration.load(std::memory_order_relaxed);
	m_commit_duration = other.m_commit_duration.load(std::memory_order_relaxed);
	m_execution_duration = other.m_execution_duration.load(std::memory_order_relaxed);
	// move axi handle
	m_axi = std::move(other.m_axi);
	// move queues
	m_receive_queue.~receive_queue_type();
	new (&m_receive_queue) decltype(m_receive_queue)(std::move(other.m_receive_queue));
	// create decoder
	m_decoder.~decoder_type();
	new (&m_decoder) decltype(m_decoder)(other.m_decoder, m_receive_queue, m_listener_halt);
	// create and start threads
	m_worker_receive = std::thread(&AXIConnection<ConnectionParameter>::work_receive, this);
	HXCOMM_LOG_TRACE(m_logger, "AXIConnection(): AXI connection startup initiated.");
}

template <typename ConnectionParameter>
AXIConnection<ConnectionParameter>& AXIConnection<ConnectionParameter>::operator=(
    AXIConnection&& other)
{
	if (&other != this) {
		// shutdown own threads
		if (m_run_receive) {
			m_run_receive = false;
			m_worker_receive.join();
		}
		m_run_receive = static_cast<bool>(other.m_run_receive);
		// shutdown other threads
		if (other.m_run_receive) {
			other.m_run_receive = false;
			other.m_worker_receive.join();
		}
		m_encode_duration = other.m_encode_duration.load(std::memory_order_relaxed);
		m_decode_duration = other.m_decode_duration.load(std::memory_order_relaxed);
		m_commit_duration = other.m_commit_duration.load(std::memory_order_relaxed);
		m_execution_duration = other.m_execution_duration.load(std::memory_order_relaxed);
		// move axi handle
		m_axi = std::move(other.m_axi);
		// move queues
		m_send_queue.~send_queue_type();
		new (&m_send_queue) send_queue_type(other.m_send_queue);
		m_receive_queue.~receive_queue_type();
		new (&m_receive_queue) decltype(m_receive_queue)(std::move(other.m_receive_queue));
		// create decoder
		m_decoder.~decoder_type();
		new (&m_decoder) decltype(m_decoder)(other.m_decoder, m_receive_queue, m_listener_halt);
		// create encoder
		m_encoder.~encoder_type();
		new (&m_encoder) encoder_type(other.m_encoder, m_send_queue);
		// create and start thread
		m_worker_receive = std::thread(&AXIConnection<ConnectionParameter>::work_receive, this);
	}
	return *this;
}


template <typename ConnectionParameter>
AXIConnection<ConnectionParameter>::~AXIConnection()
{
	HXCOMM_LOG_TRACE(m_logger, "~AXIConnection(): Stopping AXI connection.");
	if (m_run_receive) {
		m_run_receive = false;
		m_worker_receive.join();
	}
}

template <typename ConnectionParameter>
std::mutex& AXIConnection<ConnectionParameter>::get_mutex()
{
	return m_mutex;
}

template <typename ConnectionParameter>
void AXIConnection<ConnectionParameter>::add(send_message_type const& message)
{
	hate::Timer timer;
	if (!m_axi) {
		throw std::runtime_error("Unexpected access to moved-from AXIConnection.");
	}
	std::visit([this](auto const& m) { m_encoder(m); }, message);
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(duration, std::memory_order_relaxed);
	m_execution_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
template <typename InputIterator>
void AXIConnection<ConnectionParameter>::add(InputIterator const& begin, InputIterator const& end)
{
	hate::Timer timer;
	if (!m_axi) {
		throw std::runtime_error("Unexpected access to moved-from AXIConnection.");
	}
	m_encoder(begin, end);
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
	m_execution_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
void AXIConnection<ConnectionParameter>::commit()
{
	hate::Timer timer;
	m_encoder.flush();
	HXCOMM_LOG_DEBUG(m_logger, "commit(): Commiting " << m_send_queue.size() << " word(s).");
	m_axi->send(m_send_queue);
	m_commit_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

template <typename ConnectionParameter>
typename AXIConnection<ConnectionParameter>::receive_queue_type
AXIConnection<ConnectionParameter>::receive_all()
{
	receive_queue_type all;
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	std::swap(all, m_receive_queue);
	return all;
}

template <typename ConnectionParameter>
void AXIConnection<ConnectionParameter>::work_receive()
{
	while (m_run_receive) {
		auto const& data = m_axi->receive();

		HXCOMM_LOG_TRACE(m_logger, "Forwarding packet contents to decoder-coroutine..");
		hate::Timer timer;
		{
			std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
			m_decoder(data.begin(), data.end());
		}
		m_decode_duration.fetch_add(timer.get_ns(), std::memory_order_release);
		HXCOMM_LOG_TRACE(m_logger, "Forwarded packet contents to decoder-coroutine.");
	}
}

template <typename ConnectionParameter>
bool AXIConnection<ConnectionParameter>::receive_empty() const
{
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	return m_receive_queue.empty();
}

template <typename ConnectionParameter>
void AXIConnection<ConnectionParameter>::run_until_halt()
{
	using namespace std::literals::chrono_literals;

	hate::Timer timer;
	if (!m_axi) {
		throw std::runtime_error("Unexpected access to moved-from AXIConnection.");
	}

	// trigger "unhold" to start playback (if program was smaller than fifo depth)
	m_axi->trigger_unhold();

	SignalOverrideIntTerm signal_override;

	auto wait_period = 1us;
	constexpr std::chrono::microseconds max_wait_period = 10ms;
	while (!m_listener_halt.get()) {
		std::this_thread::sleep_for(wait_period);
		wait_period = std::min(wait_period * 2, max_wait_period);
	}
	m_listener_halt.reset();
	m_execution_duration += timer.get_ns();
}

template <typename ConnectionParameter>
ConnectionTimeInfo AXIConnection<ConnectionParameter>::get_time_info() const
{
	return ConnectionTimeInfo{
	    std::chrono::nanoseconds(m_encode_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_decode_duration.load(std::memory_order_acquire)),
	    std::chrono::nanoseconds(m_commit_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_execution_duration.load(std::memory_order_relaxed))};
}

template <typename ConnectionParameter>
std::string AXIConnection<ConnectionParameter>::get_unique_identifier(
    std::optional<std::string>) const
{
	if (!m_axi) {
		throw std::runtime_error("Unexpected access to empty instance.");
	}

	HXCOMM_LOG_WARN(m_logger, "AXIConnection::get_unique_identifier(): "
		"Unsupported, returning DummyData.");
	return "DummyData";
}


FrickelExtMem::FrickelExtMem(off_t offset, std::size_t length)
	: m_offset(offset), m_length(length)
{
	m_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (m_fd < 0) {
		std::stringstream ss;
		ss << "Cannot open /dev/mem: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	assert(m_offset % 4096 == 0); // FIXME
	m_extmem = mmap(0, m_length, PROT_READ | PROT_WRITE, MAP_SHARED_VALIDATE | MAP_POPULATE, m_fd, m_offset);
	if (m_extmem == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
}

FrickelExtMem::~FrickelExtMem() noexcept(false)
{
	if (m_extmem && munmap(const_cast<void*>(m_extmem), m_length) == -1) {
		std::stringstream ss;
		ss << "Cannot unmap memory: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	if (close(m_fd) == -1) {
		std::stringstream ss;
		ss << "Cannot close file descriptor: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
}

template <typename T>
void FrickelExtMem::write(off_t const offset_address, T const& data)
{
	// FIXME: we should check for container properties
	if ((data.size() * sizeof(typename T::value_type) + offset_address) > m_length) {
		std::runtime_error("Out of bounds access requested.");
	}

	// FIXME: only (volatile) uint32_t seems to work properly…
	static_assert(std::is_same_v<typename T::value_type, uint32_t>);
	std::copy(data.begin(), data.end(),
		reinterpret_cast<typename T::value_type volatile*>((char*)m_extmem + offset_address));
}

template <typename T>
T FrickelExtMem::read(off_t const offset_address, std::size_t const size)
{
	// FIXME: we should check for container properties
	if ((size * sizeof(T) + offset_address) > m_length) {
		std::runtime_error("Out of bounds access requested.");
	}

	// FIXME: only (volatile) uint32_t seems to work properly…
	static_assert(std::is_same_v<typename T::value_type, uint32_t>);
	T ret(size);
	std::copy(
		reinterpret_cast<typename T::value_type volatile*>((char*)m_extmem + offset_address),
		reinterpret_cast<typename T::value_type volatile*>((char*)m_extmem + offset_address) + size,
		ret.begin());
	return ret;
}

} // namespace hxcomm
