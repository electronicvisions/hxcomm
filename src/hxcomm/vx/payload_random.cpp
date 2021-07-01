#include "hxcomm/vx/payload_random.h"
#include "hxcomm/common/payload_random.h"
#include "hxcomm/vx/instruction/omnibus_to_fpga.h"
#include "hxcomm/vx/instruction/to_fpga_jtag.h"

namespace hxcomm::random {

typename hxcomm::vx::instruction::to_fpga_jtag::Data::Payload random_payload(
    type<typename hxcomm::vx::instruction::to_fpga_jtag::Data::Payload>, std::mt19937 gen)
{
	using namespace hxcomm::vx::instruction::to_fpga_jtag;

	std::uniform_int_distribution<uintmax_t> random_num_bits(
	    std::numeric_limits<Data::Payload::NumBits>::lowest(),
	    std::numeric_limits<Data::Payload::NumBits>::max());
	auto const num_bits = random_num_bits(gen);

	std::bernoulli_distribution random_binary;

	hate::bitset<Data::max_num_bits_payload> jtag_payload;
	for (size_t i = 0; i < jtag_payload.size; ++i) {
		jtag_payload.set(i, random_binary(gen));
	}

	bool const keep_response = random_binary(gen);

	return Data::Payload(keep_response, Data::Payload::NumBits(num_bits), jtag_payload);
}

typename hxcomm::vx::instruction::omnibus_to_fpga::Address::Payload random_payload(
    type<typename hxcomm::vx::instruction::omnibus_to_fpga::Address::Payload>, std::mt19937 gen)
{
	using namespace hxcomm::vx::instruction::omnibus_to_fpga;

	std::uniform_int_distribution<uint32_t> random_address;
	auto const address = random_address(gen);

	std::bernoulli_distribution random_binary;

	hate::bitset<sizeof(uint32_t)> byte_enables;
	for (size_t i = 0; i < byte_enables.size; ++i) {
		byte_enables.set(i, random_binary(gen));
	}

	bool const is_read = random_binary(gen);

	return Address::Payload(address, is_read, byte_enables);
}

} // namespace hxcomm::random
