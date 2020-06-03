#include "hxcomm/common/connection_parameter.h"
#include "hxcomm/common/loopbackconnection.h"
#include "hxcomm/vx/utmessage.h"
#include "hxcomm/vx/utmessage_random.h"
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>

using namespace hxcomm;
namespace bpo = boost::program_options;

using namespace hxcomm::random;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;
using send_dict = hxcomm::vx::instruction::ToFPGADictionary;

template <typename UTMessageParameter>
void throughput_measurement(size_t const num, unsigned int const seed)
{
	std::mt19937 gen(seed);

	std::stringstream ss;
	ss << "Header alignment: " << UTMessageParameter::HeaderAlignment
	   << "; subword width: " << sizeof(typename UTMessageParameter::SubwordType) * CHAR_BIT
	   << "; subpacket width: " << sizeof(typename UTMessageParameter::PhywordType) * CHAR_BIT
	   << std::endl;
	std::cout << ss.str();

	typedef LoopbackConnection<UTMessageParameter> loopback_connection_t;

	loopback_connection_t connection;

	std::vector<typename loopback_connection_t::send_message_type> instructions;
	for (size_t i = 0; i < num; ++i) {
		instructions.push_back(random_ut_message<UTMessageParameter>(gen));
	}

	size_t byte_count = 0;
	for (auto msg : instructions) {
		std::visit([&byte_count](auto&& m) { byte_count += m.word_width / CHAR_BIT; }, msg);
	}

	auto to_mega_rate = [](size_t const count, auto const dur_ms) -> double {
		return static_cast<double>(count) / static_cast<double>(dur_ms) * 1000. / 1000. / 1000.;
	};

	// adding messages to commit queue
	{
		auto const begin = std::chrono::high_resolution_clock::now();

		connection.add(instructions.begin(), instructions.end());

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
			n += connection.receive_all().size();
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
	unsigned int seed;
	// clang-format off
	bpo::options_description desc("Options");
	desc.add_options()("help", "produce help message")
	("num", bpo::value<size_t>(&num)->default_value(100000000))
	("seed", bpo::value<unsigned int>(&seed)->default_value(1234));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
	bpo::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	throughput_measurement<UTMessageParameter<8, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<8, uint32_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<8, uint16_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<8, uint8_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<8, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<7, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<6, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<5, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<4, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<3, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<2, uint64_t, uint64_t, send_dict>>(num, seed);
	throughput_measurement<UTMessageParameter<1, uint64_t, uint64_t, send_dict>>(num, seed);
}
