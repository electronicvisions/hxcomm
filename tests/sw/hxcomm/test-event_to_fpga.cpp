#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/event_to_fpga.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

template <size_t num_spikes>
std::array<typename event_to_fpga::Spike, num_spikes> random_spikes()
{
	using spike_type = event_to_fpga::Spike;
	std::array<spike_type, num_spikes> spikes;
	for (auto& spike : spikes) {
		spike = spike_type(
		    draw_ranged_non_default_value<spike_type::neuron_label_type>(0),
		    draw_ranged_non_default_value<spike_type::spl1_address_type>(0));
	}
	return spikes;
}

template <size_t num_spikes>
void spike_pack_test()
{
	typename event_to_fpga::SpikePack<num_spikes>::Payload payload(random_spikes<num_spikes>());

	auto bitstream = payload.encode();
	decltype(payload) decoded;
	decoded.decode(bitstream);
	ASSERT_EQ(payload, decoded);
}

TEST(SpikePack, EncodeDecode)
{
	spike_pack_test<1>();
	spike_pack_test<2>();
	spike_pack_test<event_to_fpga::max_num_packed>();
}
