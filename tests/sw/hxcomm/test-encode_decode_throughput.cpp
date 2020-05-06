#include <queue>
#include <gtest/gtest.h>

#include "hate/timer.h"
#include "hxcomm/common/decoder.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/vx/connection_parameter.h"
#include "hxcomm/vx/utmessage.h"
#include "hxcomm/vx/utmessage_random.h"

using namespace hxcomm::random;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;
using send_dict = hxcomm::vx::instruction::ToFPGADictionary;
using recv_dict = hxcomm::vx::instruction::FromFPGADictionary;

/**
 * Queue that stores only one element and blindly overwrites it without memory allocation.
 * Used for throughput measurement simulating a fast and non-blocking circular buffer.
 */
template <typename T>
class FastQueue
{
public:
	FastQueue() : m_data() {}

	void push(T const& data) { m_data = data; }
	void push(T&& data) { m_data = std::move(data); }

	template <typename... Args>
	void emplace(Args&&... args)
	{
		m_data = std::move(T(args...));
	}

	T& front() { return m_data; }
	T const& front() const { return m_data; }

	void pop() {}

	size_t size() { return 1; }

private:
	T m_data;
};

template <typename UTMessageParameter>
std::pair<double, double> throughput_measurement(size_t num)
{
	auto to_mega_rate = [](size_t count, auto dur_ms) -> double {
		return static_cast<double>(count) / static_cast<double>(dur_ms) * 1000. / 1000. / 1000.;
	};

	size_t byte_count = 0;

	typedef typename default_ut_message<UTMessageParameter>::message_type message_type;

	// generate random messages
	std::vector<message_type> instructions;
	for (size_t i = 0; i < num; ++i) {
		instructions.push_back(random_ut_message<UTMessageParameter>());
	}

	// calculate number of bytes of encoded messages
	for (auto msg : instructions) {
		boost::apply_visitor(
		    [&byte_count](auto&& m) { byte_count += m.word_width / CHAR_BIT; }, msg);
	}

	std::pair<double, double> mega_rates;

	// encoding
	{
		typedef FastQueue<typename UTMessageParameter::PhywordType> word_queue_type;
		word_queue_type packets;
		hxcomm::Encoder<UTMessageParameter, word_queue_type> encoder(packets);

		hate::Timer timer;

		encoder(instructions);

		auto ms = timer.get_ms();
		mega_rates.first = to_mega_rate(byte_count, ms);
	}

	// decoding
	{
		// generate encoded word stream
		std::queue<typename UTMessageParameter::PhywordType> packets;
		hxcomm::Encoder<UTMessageParameter, std::queue<typename UTMessageParameter::PhywordType>>
		    encoder(packets);
		encoder(instructions);

		// align stream in memory for easy access
		std::vector<typename UTMessageParameter::PhywordType> packets_vector;
		size_t const size = packets.size();
		for (size_t i = 0; i < size; ++i) {
			packets_vector.push_back(packets.front());
			packets.pop();
		}

		FastQueue<message_type> responses;
		hxcomm::Decoder<UTMessageParameter, decltype(responses)> decoder(responses);

		hate::Timer timer;

		decoder(packets_vector);

		auto ms = timer.get_ms();
		mega_rates.second = to_mega_rate(byte_count, ms);
	}
	return mega_rates;
}

constexpr size_t num = 1000000; // tuned so that test takes less than 30s

TEST(Encoder, Throughput)
{
	auto result = throughput_measurement<typename hxcomm::vx::ConnectionParameter::Send>(num);
	auto encode_mega_rate = result.first;
	EXPECT_GT(encode_mega_rate, 125.); // reach minimally 1GBit
}

TEST(Decoder, Throughput)
{
	auto result =
	    throughput_measurement<typename hxcomm::vx::ConnectionParameter::Receive>(num);
	auto decode_mega_rate = result.second;
	EXPECT_GT(decode_mega_rate, 125.); // reach minimally 1GBit
}
