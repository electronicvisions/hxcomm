#include "hxcomm/vx/instruction/to_fpga_jtag.h"

namespace hxcomm::vx::instruction::to_fpga_jtag {

Ins::Payload const Ins::EXTEST{0};
Ins::Payload const Ins::IDCODE{1};
Ins::Payload const Ins::SAMPLE_PRELOAD{2};
Ins::Payload const Ins::PLL_TARGET_REG{3};
Ins::Payload const Ins::SHIFT_PLL{4};
Ins::Payload const Ins::OMNIBUS_ADDRESS{5};
Ins::Payload const Ins::OMNIBUS_DATA{6};
Ins::Payload const Ins::OMNIBUS_REQUEST{7};
Ins::Payload const Ins::BYPASS{127};


Data::Payload::Payload() : m_keep_response(false), m_num_bits(), m_payload() {}
Data::Payload::Payload(
    bool const keep_response,
    NumBits const num_bits,
    hate::bitset<max_num_bits_payload> const payload) :
    m_keep_response(keep_response),
    m_num_bits(num_bits),
    m_payload(payload)
{}

bool Data::Payload::get_keep_response() const
{
	return m_keep_response;
}

void Data::Payload::set_keep_response(bool const value)
{
	m_keep_response = value;
}

Data::Payload::NumBits Data::Payload::get_num_bits() const
{
	return m_num_bits;
}

void Data::Payload::set_num_bits(NumBits const value)
{
	m_num_bits = value;
}

hate::bitset<Data::max_num_bits_payload> Data::Payload::get_payload() const
{
	return m_payload;
}

void Data::Payload::set_payload(hate::bitset<max_num_bits_payload> const& value)
{
	m_payload = value;
}

bool Data::Payload::operator==(Payload const& other) const
{
	return (
	    (m_keep_response == other.m_keep_response) && (m_num_bits == other.m_num_bits) &&
	    (m_payload == other.m_payload));
}

bool Data::Payload::operator!=(Payload const& other) const
{
	return !(*this == other);
}

} // namespace hxcomm::vx::instruction::to_fpga_jtag
