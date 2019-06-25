#include "hxcomm/vx/instruction/omnibus_to_fpga.h"

namespace hxcomm::vx::instruction::omnibus_to_fpga {

Address::Payload::Payload(
    uint32_t const address, bool const is_read, hate::bitset<sizeof(uint32_t)> byte_enables) :
    m_address(address),
    m_is_read(is_read),
    m_byte_enables(byte_enables)
{}

bool Address::Payload::operator==(Payload const& other) const
{
	return (m_is_read == other.m_is_read) && (m_address == other.m_address);
}

bool Address::Payload::operator!=(Payload const& other) const
{
	return !(*this == other);
}

} // namespace hxcomm::vx::instruction::omnibus_to_fpga
