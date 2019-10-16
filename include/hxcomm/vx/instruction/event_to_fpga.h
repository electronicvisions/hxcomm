#pragma once
#include <climits>
#include <boost/type_index.hpp>

#include "hate/join.h"
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include "hxcomm/vx/instruction/event_constants.h"

/** Instructions for events to the FPGA. */
namespace hxcomm::vx::instruction::event_to_fpga {

template <size_t num_spikes>
struct SpikePack
{
	constexpr static size_t size = event_constants::spike_size * num_spikes;

	static_assert(
	    num_spikes <= event_constants::max_num_packed, "Pack size too large, is not supported.");

	/** Payload of a SpikePack instruction. */
	class Payload
	{
	public:
		typedef hate::bitset<size> value_type;
		typedef std::array<hate::bitset<event_constants::spike_size>, num_spikes> spikes_type;

		Payload() : m_spikes() {}
		Payload(spikes_type const& spikes) : m_spikes(spikes) {}

		spikes_type const& get_spikes() const { return m_spikes; }
		void set_spikes(spikes_type const& spikes) { m_spikes = spikes; }

		bool operator==(Payload const& other) const { return m_spikes == other.m_spikes; }
		bool operator!=(Payload const& other) const { return !(*this == other); }

		template <class SubwordType = unsigned long>
		hate::bitset<size, SubwordType> encode() const
		{
			value_type ret;
			for (size_t i = 1; i <= num_spikes; ++i) {
				ret |= value_type(m_spikes[i - 1])
				       << ((num_spikes - i) * event_constants::spike_size);
			}
			return ret;
		}

		template <class SubwordType = unsigned long>
		void decode(hate::bitset<size, SubwordType> const& data)
		{
			for (size_t i = 1; i <= num_spikes; ++i) {
				m_spikes[i - 1] = typename spikes_type::value_type(
				    data >> ((num_spikes - i) * event_constants::spike_size));
			}
		}

		friend std::ostream& operator<<(std::ostream& os, Payload const& value)
		{
			os << boost::typeindex::type_id<SpikePack>().pretty_name() << "("
			   << hate::join_string(value.m_spikes, ", ") << ")";
			return os;
		}

	private:
		spikes_type m_spikes;
	};
};


namespace detail {

template <template <size_t> class Pack, typename IS>
struct GenerateDictionary;

template <template <size_t> class Pack, size_t... Is>
struct GenerateDictionary<Pack, std::index_sequence<Is...>>
{
	typedef hate::type_list<Pack<Is + 1>...> type;
};

} // namespace detail

/** Dictionary of all events to FPGA instructions. */
typedef typename detail::GenerateDictionary<
    SpikePack,
    std::make_index_sequence<event_constants::max_num_packed>>::type Dictionary;

} // namespace hxcomm::vx::instruction::event_to_fpga
