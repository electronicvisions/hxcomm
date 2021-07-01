#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/payload_random.h"
#include "hxcomm/vx/instruction/event_from_fpga.h"
#include "hxcomm/vx/instruction/event_to_fpga.h"
#include "hxcomm/vx/instruction/omnibus_to_fpga.h"
#include "hxcomm/vx/instruction/to_fpga_jtag.h"

namespace hxcomm::random {

typename hxcomm::vx::instruction::to_fpga_jtag::Data::Payload random_payload(
    type<typename hxcomm::vx::instruction::to_fpga_jtag::Data::Payload>,
    std::mt19937 gen) SYMBOL_VISIBLE;

typename hxcomm::vx::instruction::omnibus_to_fpga::Address::Payload random_payload(
    type<typename hxcomm::vx::instruction::omnibus_to_fpga::Address::Payload>,
    std::mt19937 gen) SYMBOL_VISIBLE;

template <size_t NumPack>
typename hxcomm::vx::instruction::event_to_fpga::SpikePackPayload<NumPack> random_payload(
    type<typename hxcomm::vx::instruction::event_to_fpga::SpikePackPayload<NumPack>>,
    std::mt19937 gen)
{
	using namespace hxcomm::vx::instruction::event_to_fpga;

	std::array<hate::bitset<hxcomm::vx::instruction::event_constants::spike_size>, NumPack> spikes;
	for (auto& spike : spikes) {
		std::bernoulli_distribution distribution;
		for (size_t i = 0; i < spike.size; ++i) {
			spike.set(i, distribution(gen));
		}
	}
	return spikes;
}

template <size_t NumPack>
typename hxcomm::vx::instruction::event_from_fpga::SpikePackPayload<NumPack> random_payload(
    type<typename hxcomm::vx::instruction::event_from_fpga::SpikePackPayload<NumPack>>,
    std::mt19937 gen)
{
	using namespace hxcomm::vx::instruction::event_from_fpga;

	std::array<Spike, NumPack> spikes;
	for (auto& spike : spikes) {
		std::bernoulli_distribution distribution;
		Spike::spike_type s;
		Spike::Timestamp t;
		for (size_t i = 0; i < s.size; ++i) {
			s.set(i, distribution(gen));
		}
		for (size_t i = 0; i < t.size; ++i) {
			t.set(i, distribution(gen));
		}
		spike = Spike(s, t);
	}
	return spikes;
}

template <size_t NumPack>
typename hxcomm::vx::instruction::event_from_fpga::MADCSamplePackPayload<NumPack> random_payload(
    type<typename hxcomm::vx::instruction::event_from_fpga::MADCSamplePackPayload<NumPack>>,
    std::mt19937 gen)
{
	using namespace hxcomm::vx::instruction::event_from_fpga;

	std::array<MADCSample, NumPack> madc_samples;
	for (auto& madc_sample : madc_samples) {
		std::bernoulli_distribution distribution;
		MADCSample::Value v;
		MADCSample::Timestamp t;
		for (size_t i = 0; i < v.size; ++i) {
			v.set(i, distribution(gen));
		}
		for (size_t i = 0; i < t.size; ++i) {
			t.set(i, distribution(gen));
		}
		madc_sample = MADCSample(v, t);
	}
	return madc_samples;
}

} // namespace hxcomm::random
