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
	inline AXIHandle(off_t register_addr, off_t data_addr);

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

private:
	log4cxx::Logger* m_logger;

	// file descript pointing to /dev/mem
	int m_fd;

	// page size (needed for mmap alignment purposes)
	long m_map_size;

	// mapped memory regions: use two to keep keep number of pages low
	void* m_mem; // control registers
	void* m_mem_64b; // the I/O part

	// base address of the fifos
	off_t const m_base_addr;
	off_t const m_base_addr_64b;

	// offsets from base address (cf. AXI4-Steram FIFO v4.2 Table 2-4)
	off_t static constexpr
	    offset_ISR  = 0x0,  // Interrupt Status Register
	    offset_IER  = 0x4,  // Interrupt Enable Register
	    offset_TDFR = 0x8,  // Transmit Data FIFO Reset
	    offset_TDFV = 0xc,  // Transmit Data FIFO Vacancy
	    offset_TDFD = 0x10, // Transmit Data FIFO 32-bit Data Write Port
	    offset_TLR  = 0x14, // Transmit Length Register
	    offset_RDFR = 0x18, // Read Data FIFO Reset
	    offset_RDFO = 0x1c, // Read Data FIFO Occupancy
	    offset_RDFD = 0x20, // Read Data FIFO 32-bit Data Read Port
	    offset_RLR  = 0x24, // Read Length Register
	    offset_SRR  = 0x28, // AXI4-Stream Reset
	    offset_TDR  = 0x2c, // Transmit Destination
	    offset_RDR  = 0x30  // Read Destination Register
	                        // Transmit ID Register
	                        // Transmit USER Register
	                        // Read ID Register
	                        // Read USER Register
	                        // Reserved
	;

	// offsets from base address for write/read ports
	off_t const m_offset_64b_write = 0x0;    // i.e. 0xa0020000
	off_t const m_offset_64b_read  = 0x1000; // i.e. 0xa0021000

	// 1 means interrupt pending
	union register_ISR {
		uint32_t raw;
		struct __attribute__((packed)) {
			uint32_t     : 24; // 24 bit padding
			uint32_t TRC :  1; // bit 25: transmit reset complete
			uint32_t     :  2; //  2 bit padding
			uint32_t TC  :  1; // bit 28: transmit reset complete
			uint32_t     :  4; //  4 bit padding
		} bit;
	};
	static_assert(sizeof(register_ISR::bit) == sizeof(register_ISR::raw));
	static_assert(sizeof(register_ISR) == sizeof(register_ISR::bit));

	// to mark first transaction after opening connection
	bool m_first_transaction = true;

	// read 32-bit value from mapped memory for registers
	uint32_t read_reg(off_t const& offset) {
		uint32_t ret = *(volatile uint32_t*)((char*)m_mem + offset);
		return ret;
	}

	// read 64-bit value from mapped memory for data
	uint64_t read_data(off_t const& offset) {
		// read data (64bit words) from 0xa0021000
		uint64_t ret = *(volatile uint64_t*)((char*)m_mem_64b + offset);
		return ret;
	}

	// write 32-bit register value to mapped memory for registers
	void write_reg(off_t const& offset, uint32_t const& value) {
		*(volatile uint32_t*)((char*)m_mem + offset) = value;
	}

	// write 64-bit register value to mapped memory for data
	void write_data(off_t const& offset, uint64_t const& value) {
		*(volatile uint64_t*)((char*)m_mem_64b + offset) = value;
	}
};

AXIHandle::AXIHandle(
	off_t const register_addr = 0xa0010000,
	off_t const data_addr = 0xa0020000) :
	m_logger(log4cxx::Logger::getLogger("hxcomm.AXIHandle")),
	m_base_addr(register_addr),
	m_base_addr_64b(data_addr)
{
	m_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (m_fd == -1) {
		std::stringstream ss;
		ss << "Cannot open /dev/mem: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	// need two pages due to base addresses being 4096 apart...
	m_map_size = 2 * sysconf(_SC_PAGE_SIZE);
	if (m_map_size == -1) {
		std::stringstream ss;
		ss << "Cannot acquire page size: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
	off_t const map_mask = m_map_size - 1;

	// express some assumptions
	assert((m_base_addr & ~map_mask) == m_base_addr); // alignment
	assert(m_map_size == 2 * 4096); // size of pages

	// map in single pages
	m_mem = mmap(0, m_map_size, (PROT_READ | PROT_WRITE), MAP_SHARED_VALIDATE, m_fd, m_base_addr & ~map_mask);
	if (m_mem == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap register region: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}
	m_mem_64b = mmap(0, m_map_size, (PROT_READ | PROT_WRITE), MAP_SHARED_VALIDATE, m_fd, m_base_addr_64b & ~map_mask);
	if (m_mem_64b == MAP_FAILED) {
		std::stringstream ss;
		ss << "Cannot mmap data region: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	// reset "reset registers"
	write_reg(offset_TDFR, 0xa5);
	write_reg(offset_RDFR, 0xa5);
	write_reg(offset_ISR, 0xffffffff);
	m_first_transaction = true;

	// drop all receive data
	auto drop_data = receive();
	if (!drop_data.empty()) {
		HXCOMM_LOG_WARN(m_logger, "AXIHandle(): Unexpected data in receive queue!");
	}

	assert((m_base_addr_64b + m_offset_64b_write) == 0xa0020000);
	assert((m_base_addr_64b + m_offset_64b_read) == 0xa0021000);
}

AXIHandle::~AXIHandle() noexcept(false)
{
	// exceptions trigger call to std::terminate
	if (m_mem && munmap(m_mem, m_map_size) == -1) {
		std::stringstream ss;
		ss << "Cannot unmap memory: " << std::strerror(errno);
		throw std::runtime_error(ss.str());
	}

	if (m_mem_64b && munmap(m_mem, m_map_size) == -1) {
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
		// block until ISR[TC] or ISR[TRC]
		bool ready = m_first_transaction;
		register_ISR isr;
		while (!ready) {
			isr.raw = read_reg(offset_ISR);
			if (isr.bit.TRC && (!m_first_transaction)) {
				throw std::runtime_error("Illegal ISR state");
			}
			ready = isr.bit.TRC ^ isr.bit.TC;
		}

		// enable TC and TRC in ISR
		isr.bit.TRC = 1;
		isr.bit.TC = 1;
		write_reg(offset_ISR, isr.raw);

		// might have have been first transaction => it's not anymore
		m_first_transaction = false;

		// read TDFV for number of empty slots in TX fifo
		long int vacancies = read_reg(offset_TDFV);
		if (vacancies > 1) {
			// TODO: cf. issue #3810
			vacancies = 1;
		}

		// write all data (64bit words)
		int i = 0;
		for (; (!q.empty()) && (i < vacancies); i++) {
			write_data(m_offset_64b_write, q.front());
			q.pop();
		}

		// write data length (in byte) to TLR
		write_reg(offset_TLR, 8*i);
	}
}

std::vector<AXIHandle::word_type> AXIHandle::receive()
{
	// read RLR (ignore MSB) [in bytes]; MSB indicates a partial packet
	size_t N = read_reg(offset_RLR) & 0x7FFFFFFF;
	if ((N % sizeof(AXIHandle::word_type)) != 0) {
		// we don't use byte enables, i.e. word-sized only
		throw std::runtime_error("unexpected number of bytes available");
	}
	N /= sizeof(AXIHandle::word_type);

	std::vector<AXIHandle::word_type> ret;
	ret.reserve(N);

	for (size_t i = 0; i < N; i++) {
		ret.push_back(read_data(m_offset_64b_read));
	}

	return ret;
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

} // namespace hxcomm
