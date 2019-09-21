#include <chrono>
#include <thread>
#include <gtest/gtest.h>

#include "hxcomm/common/double_buffer.h"

using namespace hxcomm;

TEST(DoubleBuffer, Throughput)
{
	constexpr size_t num = 1000000;

	auto to_kilo_rate = [](size_t count, auto dur_ms) -> double {
		return static_cast<double>(count) / static_cast<double>(dur_ms) * 1000. / 1000.;
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
	EXPECT_GT(
	    to_kilo_rate(num, std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()),
	    300.);
}
