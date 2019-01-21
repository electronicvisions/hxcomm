#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

/** Helper to asyncronously read from an input stream. */
class AsyncStreamReader
{
public:
	AsyncStreamReader(std::istream* stream) : m_stream(stream)
	{
		m_eof = false;
		m_thread = std::thread([&] { thread_entry(); });
	}

	bool is_ready()
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		return !m_queue.empty();
	}

	std::string get_line()
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		if (m_queue.empty()) {
			return {};
		}
		auto line = std::move(m_queue.front());
		m_queue.pop();
		return line;
	}

	inline bool still_running() { return !m_eof || is_ready(); }

	~AsyncStreamReader() { m_thread.join(); }

private:
	std::istream* m_stream = nullptr;
	std::atomic_bool m_eof;

	std::mutex m_mutex;

	std::queue<std::string> m_queue;

	std::thread m_thread;

	void thread_entry()
	{
		std::string line;

		while (std::getline(std::cin, line)) {
			std::lock_guard<std::mutex> lk(m_mutex);
			m_queue.push(std::move(line));
		}
		m_eof = true;
	}
};
