#include "hxcomm/vx/instruction/from_fpga_system.h"

namespace hxcomm::vx::instruction::from_fpga_system {

halt::payload_type::payload_type() : m_to_fpga_count(), m_from_fpga_count() {}
halt::payload_type::payload_type(ToFpgaCount to_fpga_count, FromFpgaCount from_fpga_count) :
    m_to_fpga_count(to_fpga_count),
    m_from_fpga_count(from_fpga_count)
{}

halt::payload_type::ToFpgaCount halt::payload_type::get_to_fpga_count() const
{
	return m_to_fpga_count;
}

void halt::payload_type::set_to_fpga_count(ToFpgaCount const value)
{
	m_to_fpga_count = value;
}

halt::payload_type::FromFpgaCount halt::payload_type::get_from_fpga_count() const
{
	return m_from_fpga_count;
}

void halt::payload_type::set_from_fpga_count(FromFpgaCount const value)
{
	m_from_fpga_count = value;
}

bool halt::payload_type::operator==(payload_type const& other) const
{
	return (
	    (m_to_fpga_count == other.m_to_fpga_count) &&
	    (m_from_fpga_count == other.m_from_fpga_count));
}

bool halt::payload_type::operator!=(payload_type const& other) const
{
	return !(*this == other);
}

} // namespace hxcomm::vx::instruction::from_fpga_system
