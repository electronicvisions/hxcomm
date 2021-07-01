#pragma once
#include "hate/visibility.h"
#include <chrono>
#include <iosfwd>

namespace cereal {
class access;
} // namespace cereal

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

	friend std::ostream& operator<<(std::ostream& os, ConnectionTimeInfo const& data)
	    SYMBOL_VISIBLE;

	ConnectionTimeInfo& operator-=(ConnectionTimeInfo const& other) SYMBOL_VISIBLE;
	ConnectionTimeInfo operator-(ConnectionTimeInfo const& other) const SYMBOL_VISIBLE;
	ConnectionTimeInfo& operator+=(ConnectionTimeInfo const& other) SYMBOL_VISIBLE;
	ConnectionTimeInfo operator+(ConnectionTimeInfo const& other) const SYMBOL_VISIBLE;

	bool operator==(ConnectionTimeInfo const& other) const SYMBOL_VISIBLE;
	bool operator!=(ConnectionTimeInfo const& other) const SYMBOL_VISIBLE;

private:
	friend cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t version);
};

} // namespace hxcomm
