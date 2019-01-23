#pragma once
#include <climits>
#include "hxcomm/common/payload.h"

/** FPGA system responses to the host. */
namespace hxcomm::vx::instruction::from_fpga_system {

/**
 * Response to a system::halt packet. It contains the message count from and to the FPGA since
 * last reset. */
struct halt
{
	constexpr static size_t size = 2 * sizeof(uint32_t) * CHAR_BIT;

	class payload_type
	{
	public:
		typedef hate::bitset<size> value_type;

		struct ToFpgaCount : public halco::common::detail::BaseType<ToFpgaCount, uint32_t>
		{
			explicit ToFpgaCount(value_type const value = 0) : base_t(value) {}
		};

		struct FromFpgaCount : public halco::common::detail::BaseType<FromFpgaCount, uint32_t>
		{
			explicit FromFpgaCount(value_type const value = 0) : base_t(value) {}
		};

		payload_type();
		payload_type(ToFpgaCount to_fpga_count, FromFpgaCount from_fpga_count);

		ToFpgaCount get_to_fpga_count() const;
		void set_to_fpga_count(ToFpgaCount value);

		FromFpgaCount get_from_fpga_count() const;
		void set_from_fpga_count(FromFpgaCount value);

		bool operator==(payload_type const& other) const;
		bool operator!=(payload_type const& other) const;

		template <typename SubwordType = unsigned long>
		hate::bitset<size, SubwordType> encode() const
		{
			return (
			    value_type(m_to_fpga_count) << (sizeof(FromFpgaCount::value_type) * CHAR_BIT) |
			    value_type(m_from_fpga_count));
		}

		template <typename SubwordType = unsigned long>
		void decode(hate::bitset<size, SubwordType> const& data)
		{
			m_to_fpga_count = ToFpgaCount(static_cast<ToFpgaCount::value_type>(
			    data >> sizeof(FromFpgaCount::value_type) * CHAR_BIT));
			m_from_fpga_count = FromFpgaCount(static_cast<FromFpgaCount::value_type>(data));
		}

	private:
		ToFpgaCount m_to_fpga_count;
		FromFpgaCount m_from_fpga_count;
	};
};

/** Dictionary of all FPGA system response instructions. */
typedef hate::type_list<halt> dictionary;

} // namespace hxcomm::vx::instruction::from_fpga_system
