#include <gtest/gtest.h>

#include "hxcomm/vx/instruction/event_to_fpga.h"

#include "test-helper.h"

using namespace hxcomm::vx::instruction;

template <size_t num_spikes>
typename event_to_fpga::SpikePack<num_spikes>::Payload::spikes_type random_spikes()
{
	typename event_to_fpga::SpikePack<num_spikes>::Payload::spikes_type spikes;
	for (auto& spike : spikes) {
		spike = random_bitset<event_constants::spike_size>();
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
	spike_pack_test<event_constants::max_num_packed>();
}
