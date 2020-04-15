#pragma once

#include <chrono>

#include "RCF/RCF.hpp"

namespace hxcomm {

using namespace std::chrono_literals;

/**
 * Wrapper class that holds an async-response as well as the corresponding
 * RCF-client because each RCF-client only supponse one concurrent asynchronous
 * call.
 *
 * If forwards needed portions of the RCF::Future-API.
 */
template <typename ReturnValue, typename RcfClient>
class QuiggeldyFuture
{
public:
	using rcf_client_type = RcfClient;
	using return_value_type = ReturnValue;
	using rcf_future_type = RCF::Future<ReturnValue>;

	/**
	 * Construct QuiggeldyFuture.
	 *
	 * @param client Client used to create future.
	 * @param future RCF::Future value to be wrapped.
	 */
	QuiggeldyFuture(
	    std::unique_ptr<rcf_client_type> client, std::unique_ptr<rcf_future_type> future) :
	    m_client(std::move(client)), m_future(std::move(future)){};

	/**
	 * Check if the Future holds a value.
	 */
	bool ready()
	{
		if (m_future) {
			return m_future->ready();
		} else {
			return false;
		}
	}

	/**
	 * Cancel the future and clear this instance.
	 */
	void cancel()
	{
		if (m_future) {
			m_future->cancel();
		}

		m_future.reset();
		m_client.reset();
	}

	/**
	 * Access the computed value, blocking until the value is ready. If
	 * QuiggeldyFuture was `cancel()`-ed, will throw.
	 *
	 * Should only be called once!
	 */
	return_value_type operator*()
	{
		if (!m_future || !m_client) {
			throw std::runtime_error("Trying to retrieve value of an empty QuiggeldyFuture.");
		}
		if (!m_future->ready()) {
			m_future->wait();
		}
		auto retval = **m_future;
		m_future.reset();
		if (m_client) {
			// reset client so that we do not clog connections upstream
			m_client.reset();
		}
		return retval;
	}

	/**
	 * Wait for the future optionally with a timeout.
	 */
	void wait(std::chrono::milliseconds timeout = 0ms)
	{
		m_future->wait(timeout.count());
	}

private:
	std::unique_ptr<rcf_client_type> m_client;
	std::unique_ptr<RCF::Future<return_value_type>> m_future;
};

} // namespace hxcomm
