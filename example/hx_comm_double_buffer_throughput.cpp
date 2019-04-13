#include <chrono>
#include <iostream>
#include <thread>
#include <boost/program_options.hpp>


#include "hxcomm/common/double_buffer.h"

using namespace hxcomm;
namespace bpo = boost::program_options;

void throughput_measurement(size_t num)
{
	auto to_mega_rate = [](size_t count, auto dur_ms) -> double {
		return static_cast<double>(count) / static_cast<double>(dur_ms) * 1000. / 1000. / 1000.;
	};

	std::atomic<bool> run = true;
	DoubleBuffer<Packet<int, 1>> buffer(run);

	auto begin = std::chrono::high_resolution_clock::now();

	auto producer = std::thread([&run, &buffer]() {
		while (true) {
			auto ptr = buffer.start_write();
			if (!ptr) {
				buffer.notify();
				return;
			}
			buffer.stop_write();
		}
	});

	std::atomic<size_t> count = 0;
	auto consumer = std::thread([&run, &count, &buffer]() {
		while (true) {
			auto ptr = buffer.start_read();
			if (!ptr) {
				buffer.notify();
				return;
			}
			buffer.stop_read();
			count = count + 1;
		}
	});

	while (count < num) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	auto end = std::chrono::high_resolution_clock::now();

	run = false;
	buffer.notify();
	producer.join();
	consumer.join();

	auto dur = end - begin;
	std::cout << "Packets/s: "
	          << to_mega_rate(
	                 num, std::chrono::duration_cast<std::chrono::milliseconds>(dur).count())
	          << " MHz" << std::endl;
}

int main(int argc, char* argv[])
{
	// parse arguments
	size_t num;
	// clang-format off
	bpo::options_description desc("Options");
	desc.add_options()("help", "produce help message")
	("num", bpo::value<size_t>(&num)->default_value(1000000));
	// clang-format on

	bpo::variables_map vm;
	bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
	bpo::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	throughput_measurement(num);
}
