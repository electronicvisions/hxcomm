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
using send_dict = hxcomm::vx::instruction::to_fpga_dictionary;

/** Return default-constructed ut_message of runtime-specifiable header. */
template <typename UTMessageParameter, class SubpacketType>
struct default_message
{
	typedef typename LoopbackConnection<UTMessageParameter, SubpacketType>::receive_message_type
	    message_type;

	template <size_t H, size_t... Hs>
	static message_type message_recurse(size_t const header, std::index_sequence<H, Hs...>)
	{
		return (header == H) ? ut_message<
		                           UTMessageParameter::HeaderAlignment,
		                           typename UTMessageParameter::SubwordType,
		                           typename UTMessageParameter::Dictionary,
		                           typename hate::index_type_list_by_integer<
		                               H, typename UTMessageParameter::Dictionary>::type>()
		                     : message_recurse(header, std::index_sequence<Hs...>());
	}

	template <size_t H>
	static message_type message_recurse(size_t const header, std::index_sequence<H>)
	{
		if (header == H) {
			return ut_message<
			    UTMessageParameter::HeaderAlignment, typename UTMessageParameter::SubwordType,
			    typename UTMessageParameter::Dictionary,
			    typename hate::index_type_list_by_integer<
			        H, typename UTMessageParameter::Dictionary>::type>();
		}
		throw std::runtime_error(
		    "Trying to create a default cosntructed UT message with unknown header.");
	}

	static message_type message(size_t header)
	{
		return message_recurse(
		    header, std::make_index_sequence<
		                hate::type_list_size<typename UTMessageParameter::Dictionary>::value>());
	}
};

template <typename UTMessageParameter, typename SubpacketType>
void throughput_measurement(size_t const num, bool const random, unsigned int const seed)
{
	srand(seed);

	std::stringstream ss;
	ss << "Header alignment: " << UTMessageParameter::HeaderAlignment
	   << "; subword width: " << sizeof(typename UTMessageParameter::SubwordType) * 8
	   << "; subpacket width: " << sizeof(SubpacketType) * 8 << std::endl;
	std::cout << ss.str();

	typedef LoopbackConnection<UTMessageParameter, SubpacketType> loopback_connection_t;

	loopback_connection_t connection;

	std::vector<typename loopback_connection_t::send_message_type> instructions;
	for (size_t i = 0; i < num; ++i) {
		instructions.push_back(default_message<UTMessageParameter, SubpacketType>::message(
		    random ? (rand() % hate::type_list_size<send_dict>::value) : 1));
	}

	size_t byte_count = 0;
	for (auto msg : instructions) {
		boost::apply_visitor(
		    [&byte_count](auto&& m) { byte_count += m.word_width / CHAR_BIT; }, msg);
	}

	auto to_mega_rate = [](size_t const count, auto const dur_ms) -> double {
		return static_cast<double>(count) / static_cast<double>(dur_ms) * 1000. / 1000. / 1000.;
	};

	// adding messages to commit queue
	{
		auto const begin = std::chrono::high_resolution_clock::now();

		connection.add(instructions);

		auto const end = std::chrono::high_resolution_clock::now();
		auto const dur = end - begin;
		auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

		std::cout << " - add: " << std::setprecision(5) << to_mega_rate(num, ms) << " MHz;\t"
		          << to_mega_rate(byte_count, ms) << " MiB/s" << std::endl;
	}
	// receiving messages from receive queue
	{
		size_t n = 0;
		typename decltype(connection)::receive_message_type message;

		auto const begin = std::chrono::high_resolution_clock::now();

		connection.commit();
		while (n < num) {
			while (connection.try_receive(message)) {
				n++;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(10000));
		}

		auto const end = std::chrono::high_resolution_clock::now();
		auto const dur = end - begin;
		auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

		std::cout << " - rec: " << std::setprecision(5) << to_mega_rate(num, ms) << " MHz;\t"
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

	throughput_measurement<UTMessageParameter<8, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<8, uint32_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<8, uint16_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<8, uint8_t, send_dict>, uint64_t>(num, random, seed);

	throughput_measurement<UTMessageParameter<8, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<7, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<6, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<5, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<4, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<3, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<2, uint64_t, send_dict>, uint64_t>(num, random, seed);
	throughput_measurement<UTMessageParameter<1, uint64_t, send_dict>, uint64_t>(num, random, seed);
}
