#pragma once
#include "hate/type_index.h"
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include <climits>

/** Instructions for omnibus communication to the FPGA. */
namespace hxcomm::vx::instruction::omnibus_to_fpga {

/**
 * Instruction to set Omnibus address for a read or write operation.
 * On read this instruction leads to a read response. On write, data is to be provided with a
 * following 'Data' instruction. To the FPGA, byte enables can be used to select byte transmission.
 */
struct Address
{
	constexpr static size_t size = 37;
	class Payload
	{
	public:
		explicit Payload(
		    uint32_t const address = 0,
		    bool const is_read = false,
		    hate::bitset<sizeof(uint32_t)> const byte_enables = 0xf) :
		    m_address(address),
		    m_is_read(is_read),
		    m_byte_enables(byte_enables)
		{}

		bool operator==(Payload const& other) const
		{
			return (m_is_read == other.m_is_read) && (m_address == other.m_address) &&
			       (m_byte_enables == other.m_byte_enables);
		}

		bool operator!=(Payload const& other) const { return !(*this == other); }

		template <class SubwordType = unsigned long>
		hate::bitset<size, SubwordType> encode() const
		{
			hate::bitset<size, SubwordType> tmp(m_address);
			tmp.set(sizeof(uint32_t) * CHAR_BIT + sizeof(uint32_t), m_is_read);
			tmp |= hate::bitset<size, SubwordType>(m_byte_enables) << sizeof(uint32_t) * CHAR_BIT;
			return tmp;
		}

		template <class SubwordType = unsigned long>
		void decode(hate::bitset<size, SubwordType> const& data)
		{
			m_address = static_cast<uint32_t>(data);
			m_is_read = data.test(sizeof(uint32_t) * CHAR_BIT + sizeof(uint32_t));
			m_byte_enables = (data >> (sizeof(uint32_t) * CHAR_BIT));
		}

		friend std::ostream& operator<<(std::ostream& os, Payload const& value)
		{
			os << hate::full_name<Address>() << "(is_read: ";
			os << std::boolalpha << value.m_is_read << ", byte_enables: " << value.m_byte_enables
			   << ", address: " << value.m_address << ")";
			return os;
		}

		bool get_is_read() const
		{
			return m_is_read;
		}

		void set_is_read(bool value)
		{
			m_is_read = value;
		}

	private:
		uint32_t m_address;
		bool m_is_read;
		hate::bitset<sizeof(uint32_t)> m_byte_enables;
	};
};

/**
 * Write data to a beforehand specified Omnibus address.
 * Write response filtering can be disabled by setting the lowest bit of omnibus on FPGA address 0
 * to false.
 */
struct Data
{
	constexpr static size_t size = sizeof(uint32_t) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::Number<Data, uint32_t> Payload;
};

/** Dictionary of all Omnibus to FPGA instructions. */
typedef hate::type_list<Address, Data> Dictionary;

} // namespace hxcomm::vx::instruction::omnibus_to_fpga
