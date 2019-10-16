#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/event_from_fpga.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

template <size_t num_madc_samples>
std::array<event_from_fpga::MADCSample, num_madc_samples> random_madc_samples()
{
	using sample_type = event_from_fpga::MADCSample;
	std::array<sample_type, num_madc_samples> madc_samples;
	for (auto& sample : madc_samples) {
		sample = sample_type(
		    draw_ranged_non_default_value<sample_type::Value>(0),
		    draw_ranged_non_default_value<sample_type::Timestamp>(0));
	}
	return madc_samples;
}

template <size_t num_madc_samples>
void madc_sample_pack_test()
{
	typename event_from_fpga::MADCSamplePack<num_madc_samples>::Payload payload(
	    random_madc_samples<num_madc_samples>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(MADCSamplePack, EncodeDecode)
{
	madc_sample_pack_test<1>();
	madc_sample_pack_test<2>();
	madc_sample_pack_test<event_constants::max_num_packed>();
}

template <size_t num_spikes>
std::array<event_from_fpga::Spike, num_spikes> random_spikes()
{
	using spike_type = event_from_fpga::Spike;
	std::array<spike_type, num_spikes> spikes;
	for (auto& spike : spikes) {
		spike = spike_type(
		    random_bitset<event_constants::spike_size>(),
		    draw_ranged_non_default_value<spike_type::Timestamp>(0));
	}
	return spikes;
}

template <size_t num_spikes>
void spike_pack_test()
{
	typename event_from_fpga::SpikePack<num_spikes>::Payload payload(random_spikes<num_spikes>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(FromFPGASpikePack, EncodeDecode)
{
	spike_pack_test<1>();
	spike_pack_test<2>();
	spike_pack_test<event_constants::max_num_packed>();
}
