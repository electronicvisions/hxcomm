#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>

#include "hxcomm/common/connection_parameter.h"
#include "hxcomm/common/loopbackconnection.h"
#include "hxcomm/vx/utmessage.h"

using namespace hxcomm;
namespace bpo = boost::program_options;

using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;
using send_dict = hxcomm::vx::instruction::ToFPGADictionary;

/** Return default-constructed UTMessage of runtime-specifiable header. */
template <typename UTMessageParameter>
struct default_message
{
	typedef typename LoopbackConnection<UTMessageParameter>::receive_message_type message_type;

	template <size_t H, size_t... Hs>
	static message_type message_recurse(size_t header, std::index_sequence<H, Hs...>)
	{
		return (header == H) ? UTMessage<
		                           UTMessageParameter::HeaderAlignment,
		                           typename UTMessageParameter::SubwordType,
		                           typename UTMessageParameter::PhywordType,
		                           typename UTMessageParameter::Dictionary,
		                           typename hate::index_type_list_by_integer<
		                               H, typename UTMessageParameter::Dictionary>::type>()
		                     : message_recurse(header, std::index_sequence<Hs...>());
	}

	template <size_t H>
	static message_type message_recurse(size_t /*header*/, std::index_sequence<H>)
	{
		return UTMessage<
		    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType,
		    typename UTMessageParameter::PhywordType, typename UTMessageParameter::Dictionary,
		    typename hate::index_type_list_by_integer<
		        H, typename UTMessageParameter::Dictionary>::type>();
	}

	static message_type message(size_t header)
	{
		return message_recurse(
		    header, std::make_index_sequence<
		                hate::type_list_size<typename UTMessageParameter::Dictionary>::value>());
	}
};

/**
 * Queue that stores only one element and blindly overwrites it without memory allocation.
 * Used for throughput measurement simulating a fast and non-blocking circular buffer.
 */
template <typename T>
class FastQueue
{
public:
	FastQueue() {}

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
void throughput_measurement(size_t num, bool random_header, unsigned int seed)
{
	srand(seed);

	std::stringstream ss;
	ss << "Header alignment: " << UTMessageParameter::HeaderAlignment
	   << "; subword width: " << sizeof(typename UTMessageParameter::SubwordType) * CHAR_BIT
	   << "; subpacket width: " << sizeof(typename UTMessageParameter::PhywordType) * CHAR_BIT
	   << std::endl;
	std::cout << ss.str();

	auto to_mega_rate = [](size_t count, auto dur_ms) -> double {
		return static_cast<double>(count) / static_cast<double>(dur_ms) * 1000. / 1000. / 1000.;
	};
	size_t byte_count = 0;

	typedef typename default_message<UTMessageParameter>::message_type message_type;

	// generate random messages
	std::vector<message_type> instructions;
	for (size_t i = 0; i < num; ++i) {
		instructions.push_back(default_message<UTMessageParameter>::message(
		    random_header ? rand() % hate::type_list_size<send_dict>::value : 1));
	}

	// calculate number of bytes of encoded messages
	for (auto msg : instructions) {
		boost::apply_visitor(
		    [&byte_count](auto&& m) { byte_count += m.word_width / CHAR_BIT; }, msg);
	}

	// encoding
	{
		typedef FastQueue<typename UTMessageParameter::PhywordType> word_queue_type;
		word_queue_type packets;
		Encoder<UTMessageParameter, word_queue_type> encoder(packets);

		auto begin = std::chrono::high_resolution_clock::now();

		encoder(instructions);

		auto end = std::chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

		std::cout << " - encode: " << std::setprecision(5) << to_mega_rate(num, ms) << " MHz;\t"
		          << to_mega_rate(byte_count, ms) << " MiB/s" << std::endl;
	}
	// decoding
	{
		// generate encoded word stream
		std::queue<typename UTMessageParameter::PhywordType> packets;
		Encoder<UTMessageParameter, std::queue<typename UTMessageParameter::PhywordType>> encoder(
		    packets);
		encoder(instructions);

		// align stream in memory for easy access
		std::vector<typename UTMessageParameter::PhywordType> packets_vector;
		size_t const size = packets.size();
		for (size_t i = 0; i < size; ++i) {
			packets_vector.push_back(packets.front());
			packets.pop();
		}

		FastQueue<message_type> responses;
		Decoder<UTMessageParameter, decltype(responses)> decoder(responses);

		auto begin = std::chrono::high_resolution_clock::now();

		decoder(packets_vector);

		auto end = std::chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

		std::cout << " - decode: " << std::setprecision(5) << to_mega_rate(num, ms) << " MHz;\t"
		          << to_mega_rate(byte_count, ms) << " MiB/s" << std::endl;
	}
}

int main(int argc, char* argv[])
{
	// parse arguments
	size_t num;
	bool random;
	unsigned int seed;
	// clang-format off
	bpo::options_description desc("Options");
	desc.add_options()("help", "produce help message")
	("num", bpo::value<size_t>(&num)->default_value(100000000))
	("random", bpo::value<bool>(&random)->default_value(true))
	("seed", bpo::value<unsigned int>(&seed)->default_value(1234));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
	bpo::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	throughput_measurement<UTMessageParameter<8, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<8, uint32_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<8, uint16_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<8, uint8_t, uint64_t, send_dict>>(num, random, seed);

	throughput_measurement<UTMessageParameter<8, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<7, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<6, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<5, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<4, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<3, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<2, uint64_t, uint64_t, send_dict>>(num, random, seed);
	throughput_measurement<UTMessageParameter<1, uint64_t, uint64_t, send_dict>>(num, random, seed);
}
