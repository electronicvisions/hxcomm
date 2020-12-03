#include "hxcomm/common/execute_messages.h"
#include "hxcomm/vx/utmessage.h"
#include "hxcomm/vx/utmessage_random.h"
#include "hxcomm/vx/zeromockconnection.h"
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/program_options.hpp>

using namespace hxcomm;
namespace bpo = boost::program_options;

using namespace hxcomm::random;
using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

typedef hxcomm::UTMessageParameter<
    ut_message_to_fpga_header_alignment,
    ut_message_to_fpga_subword_type,
    ut_message_to_fpga_phyword_type,
    ToFPGADictionary>
    UTMessageToFPGAParameter;

bool is_read(UTMessageToFPGAVariant const& instruction)
{
	if (std::holds_alternative<UTMessageToFPGA<system::Loopback>>(instruction)) {
		return true;
	} else if (std::holds_alternative<UTMessageToFPGA<to_fpga_jtag::Data>>(instruction)) {
		return std::get<UTMessageToFPGA<to_fpga_jtag::Data>>(instruction)
		    .get_payload()
		    .test(to_fpga_jtag::Data::size - to_fpga_jtag::Data::padded_num_bits_keep_response);
	} else if (std::holds_alternative<UTMessageToFPGA<omnibus_to_fpga::Address>>(instruction)) {
		return std::get<UTMessageToFPGA<omnibus_to_fpga::Address>>(instruction)
		    .get_payload()
		    .test(4 * 9);
	}
	return false;
}

std::vector<std::chrono::microseconds> single_measurement(
    size_t const num_ins,
    size_t const num_avg,
    unsigned int const seed,
    size_t const us_per_message)
{
	std::mt19937 gen(seed);

	std::vector<UTMessageToFPGAVariant> instructions;
	for (size_t i = 0; i < num_ins; ++i) {
		auto instruction = random_ut_message<UTMessageToFPGAParameter>(gen);
		while (is_read(instruction)) {
			instruction = random_ut_message<UTMessageToFPGAParameter>(gen);
		}
		if (std::holds_alternative<UTMessageToFPGA<system::Loopback>>(instruction)) {
			instructions.push_back(UTMessageToFPGA<system::Loopback>(system::Loopback::tick));
		} else {
			instructions.push_back(instruction);
		}
	}

	hxcomm::vx::ZeroMockConnection connection(us_per_message);

	std::vector<std::chrono::microseconds> durations(num_avg);
	for (size_t i = 0; i < num_avg; ++i) {
		[[maybe_unused]] auto const [result, times] =
		    hxcomm::execute_messages(connection, instructions);
		durations.at(i) =
		    std::chrono::duration_cast<std::chrono::microseconds>(times.execution_duration);
	}
	return durations;
}

/**
 * Measure the processing speed average and standard deviation of the ZeroMockConnection
 * for varying us_per_message [1,16] and concurrent use of multiple connections
 * [1,2*hw_concurrency].
 */
int main(int argc, char* argv[])
{
	// parse arguments
	size_t num;
	size_t num_avg;
	unsigned int seed;
	// clang-format off
	bpo::options_description desc("Options");
	desc.add_options()("help", "produce help message")
	("num", bpo::value<size_t>(&num)->default_value(100000))
	("num_avg", bpo::value<size_t>(&num_avg)->default_value(1000))
	("seed", bpo::value<unsigned int>(&seed)->default_value(1234));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
	bpo::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	boost::accumulators::accumulator_set<
	    double, boost::accumulators::features<
	                boost::accumulators::tag::mean, boost::accumulators::tag::variance>>
	    acc;

	size_t const max_num_threads = std::thread::hardware_concurrency();
	for (size_t num_threads = 1; num_threads <= 2 * max_num_threads; ++num_threads) {
		for (size_t us_per_message = 1; us_per_message < 16; ++us_per_message) {
			std::vector<std::future<std::vector<std::chrono::microseconds>>> futures;
			for (size_t i = 0; i < num_threads; ++i) {
				futures.push_back(std::async(std::launch::async, [&]() {
					return single_measurement(num, num_avg, seed, us_per_message);
				}));
			}
			for (auto& f : futures) {
				auto const durations = f.get();
				for (auto const& duration : durations) {
					acc(duration.count());
				}
			}
			auto const mean = boost::accumulators::mean(acc);
			auto const std = std::sqrt(boost::accumulators::variance(acc));
			acc = {};
			std::cout << num_threads << ", " << us_per_message << ": " << mean << ", " << std
			          << std::endl;
		}
	}
}
