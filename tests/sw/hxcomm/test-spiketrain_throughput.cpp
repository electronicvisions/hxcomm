#include "hate/timer.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/vx/utmessage.h"
#include <vector>
#include <gtest/gtest.h>

using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;


TEST(UTMessage, SpikeTrainCopyThroughput)
{
	auto logger = log4cxx::Logger::getLogger("hxcomm.swtest.UTMessage.SpikeTrainCopyThroughput");
	double rate_mhz = 0.;
	constexpr size_t max_pow = 25;
	for (size_t p = 0; p < max_pow; ++p) {
		size_t const num = hate::math::pow(2, p);

		std::vector<UTMessageToFPGAVariant> messages;

		hate::Timer timer;
		UTMessageToFPGAVariant const timer_setup{
		    UTMessageToFPGA<timing::Setup>{timing::Setup::Payload{}}};
		messages.push_back(timer_setup);
		for (size_t i = 0; i < num; ++i) {
			UTMessageToFPGAVariant const timer_wait{UTMessageToFPGA<timing::WaitUntil>{
			    timing::WaitUntil::Payload{static_cast<uint32_t>(i)}}};
			messages.push_back(timer_wait);
			UTMessageToFPGAVariant const spike{
			    UTMessageToFPGA<event_to_fpga::SpikePack<1>>{event_to_fpga::SpikePack<1>::Payload{
			        event_to_fpga::SpikePack<1>::Payload::spikes_type{
			            event_to_fpga::SpikePack<1>::Payload::spikes_type::value_type{i}}}}};
			messages.push_back(spike);
		}
		rate_mhz = static_cast<double>(num) / static_cast<double>(timer.get_us());
		HXCOMM_LOG_INFO(
		    logger, num << ": " << timer.print() << ", " << rate_mhz << " MHz, "
		                << (rate_mhz * 2 * sizeof(UTMessageToFPGAVariant)) << " MB/s");
	}
	EXPECT_GE(rate_mhz, 20.);
}

TEST(UTMessage, SpikeTrainEmplaceThroughput)
{
	auto logger = log4cxx::Logger::getLogger("hxcomm.swtest.UTMessage.SpikeTrainEmplaceThroughput");
	double rate_mhz = 0.;
	constexpr size_t max_pow = 25;
	for (size_t p = 0; p < max_pow; ++p) {
		size_t const num = hate::math::pow(2, p);

		std::vector<UTMessageToFPGAVariant> messages;

		hate::Timer timer;
		messages.emplace_back(UTMessageToFPGA<timing::Setup>{timing::Setup::Payload{}});
		for (size_t i = 0; i < num; ++i) {
			messages.emplace_back(UTMessageToFPGA<timing::WaitUntil>{
			    timing::WaitUntil::Payload{static_cast<uint32_t>(i)}});
			messages.emplace_back(
			    UTMessageToFPGA<event_to_fpga::SpikePack<1>>{event_to_fpga::SpikePack<1>::Payload{
			        event_to_fpga::SpikePack<1>::Payload::spikes_type{
			            event_to_fpga::SpikePack<1>::Payload::spikes_type::value_type{i}}}});
		}
		rate_mhz = static_cast<double>(num) / static_cast<double>(timer.get_us());
		HXCOMM_LOG_INFO(
		    logger,
		    num << ": " << timer.print() << ", " << rate_mhz << " MHz, "
		        << (rate_mhz * 2 * sizeof(UTMessageToFPGAVariant)) << " MB/s, "
		        << (static_cast<double>(num) * 2 * sizeof(UTMessageToFPGAVariant)) / 1024 / 1024
		        << " MB");
	}
	EXPECT_GE(rate_mhz, 25.);
}

TEST(UTMessage, SpikeTrainPreallocatedEmplaceThroughput)
{
	auto logger = log4cxx::Logger::getLogger(
	    "hxcomm.swtest.UTMessage.SpikeTrainPreallocatedEmplaceThroughput");
	double rate_mhz = 0.;
	constexpr size_t max_pow = 25;
	for (size_t p = 0; p < max_pow; ++p) {
		size_t const num = hate::math::pow(2, p);

		std::vector<UTMessageToFPGAVariant> messages;

		hate::Timer timer;
		messages.reserve(num * 2 + 1);
		messages.emplace_back(UTMessageToFPGA<timing::Setup>{timing::Setup::Payload{}});
		for (size_t i = 0; i < num; ++i) {
			messages.emplace_back(UTMessageToFPGA<timing::WaitUntil>{
			    timing::WaitUntil::Payload{static_cast<uint32_t>(i)}});
			messages.emplace_back(
			    UTMessageToFPGA<event_to_fpga::SpikePack<1>>{event_to_fpga::SpikePack<1>::Payload{
			        event_to_fpga::SpikePack<1>::Payload::spikes_type{
			            event_to_fpga::SpikePack<1>::Payload::spikes_type::value_type{i}}}});
		}
		rate_mhz = static_cast<double>(num) / static_cast<double>(timer.get_us());
		HXCOMM_LOG_INFO(
		    logger,
		    num << ": " << timer.print() << ", " << rate_mhz << " MHz, "
		        << (rate_mhz * 2 * sizeof(UTMessageToFPGAVariant)) << " MB/s, "
		        << (static_cast<double>(num) * 2 * sizeof(UTMessageToFPGAVariant)) / 1024 / 1024
		        << " MB");
	}
	EXPECT_GE(rate_mhz, 70.);
}
