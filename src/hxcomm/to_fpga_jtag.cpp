#include "hxcomm/vx/instruction/to_fpga_jtag.h"

namespace hxcomm::vx::instruction::to_fpga_jtag {

ins::payload_type const ins::EXTEST{0};
ins::payload_type const ins::IDCODE{1};
ins::payload_type const ins::SAMPLE_PRELOAD{2};
ins::payload_type const ins::PLL_TARGET_REG{3};
ins::payload_type const ins::SHIFT_PLL{4};
ins::payload_type const ins::OMNIBUS_ADDRESS{5};
ins::payload_type const ins::OMNIBUS_DATA{6};
ins::payload_type const ins::OMNIBUS_REQUEST{7};
ins::payload_type const ins::BYPASS{127};


data::payload_type::payload_type() : m_keep_response(false), m_num_bits(), m_payload() {}
data::payload_type::payload_type(
    bool const keep_response,
    NumBits const num_bits,
    hate::bitset<max_num_bits_payload> const payload) :
    m_keep_response(keep_response),
    m_num_bits(num_bits),
    m_payload(payload)
{}

bool data::payload_type::get_keep_response() const
{
	return m_keep_response;
}

void data::payload_type::set_keep_response(bool const value)
{
	m_keep_response = value;
}

data::payload_type::NumBits data::payload_type::get_num_bits() const
{
	return m_num_bits;
}

void data::payload_type::set_num_bits(NumBits const value)
{
	m_num_bits = value;
}

hate::bitset<data::max_num_bits_payload> data::payload_type::get_payload() const
{
	return m_payload;
}

void data::payload_type::set_payload(hate::bitset<max_num_bits_payload> const& value)
{
	m_payload = value;
}

bool data::payload_type::operator==(payload_type const& other) const
{
	return (
	    (m_keep_response == other.m_keep_response) && (m_num_bits == other.m_num_bits) &&
	    (m_payload == other.m_payload));
}

bool data::payload_type::operator!=(payload_type const& other) const
{
	return !(*this == other);
}

} // namespace hxcomm::vx::instruction::to_fpga_jtag
