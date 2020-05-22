#pragma once
#include <chrono>
#include <mutex>
#include <ostream>

namespace hxcomm {

/**
 * Time information of a connection's history of usage.
 */
struct ConnectionTimeInfo
{
	/**
	 * Time spent encoding UT messages to a stream of words since construction.
	 */
	std::chrono::nanoseconds encode_duration{};

	/**
	 * Time spent decoding UT messages from a stream of words since construction.
	 */
	std::chrono::nanoseconds decode_duration{};

	/**
	 * Time spent sending streams of words to backend since construction.
	 */
	std::chrono::nanoseconds commit_duration{};

	/**
	 * Time spent waiting for end of execution on backend.
	 * For hardware execution this duration includes the commit time, since control flow for delayed
	 * begin of execution is not implemented currently.
	 */
	std::chrono::nanoseconds execution_duration{};

	friend std::ostream& operator<<(std::ostream& os, ConnectionTimeInfo const& data);

	ConnectionTimeInfo& operator-=(ConnectionTimeInfo const& other);
	ConnectionTimeInfo operator-(ConnectionTimeInfo const& other) const;
	ConnectionTimeInfo& operator+=(ConnectionTimeInfo const& other);
	ConnectionTimeInfo operator+(ConnectionTimeInfo const& other) const;

	bool operator==(ConnectionTimeInfo const& other) const;
	bool operator!=(ConnectionTimeInfo const& other) const;
};

} // namespace hxcomm

#include "hxcomm/common/connection_time_info.tcc"
