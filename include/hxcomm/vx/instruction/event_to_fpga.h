#pragma once
#include <climits>

#include "hxcomm/common/payload.h"

#include "halco/hicann-dls/vx/coordinates.h"

/** Instructions for events to the FPGA. */
namespace hxcomm::vx::instruction::event_to_fpga {

constexpr size_t max_num_packed = 3;

/** Type of one spike event. */
class Spike
{
public:
	constexpr static size_t neuron_address_size = 14;
	constexpr static size_t spl1_address_size = 2;
	constexpr static size_t size = neuron_address_size + spl1_address_size;

	typedef hate::bitset<size> value_type;

	typedef halco::hicann_dls::vx::NeuronLabel neuron_label_type;
	typedef halco::hicann_dls::vx::SPL1Address spl1_address_type;

	Spike() : m_neuron(), m_spl1() {}
	Spike(neuron_label_type const neuron, spl1_address_type spl1) : m_neuron(neuron), m_spl1(spl1)
	{}

	bool operator==(Spike const& other) const
	{
		return (m_neuron == other.m_neuron) && m_spl1 == other.m_spl1;
	}

	bool operator!=(Spike const& other) const { return !(*this == other); }

	template <class SubwordType = unsigned long>
	hate::bitset<size, SubwordType> encode() const
	{
		return (value_type(m_spl1) << neuron_address_size) | value_type(m_neuron);
	}

	template <class SubwordType = unsigned long>
	void decode(hate::bitset<size, SubwordType> const& data)
	{
		m_spl1 = spl1_address_type(static_cast<uintmax_t>(data >> neuron_address_size));
		m_neuron = neuron_label_type(
		    static_cast<uintmax_t>(hate::bitset<neuron_address_size, SubwordType>(data)));
	}

private:
	neuron_label_type m_neuron;
	spl1_address_type m_spl1;
};


template <size_t num_spikes>
struct SpikePack
{
	constexpr static size_t size = Spike::size * num_spikes;

	static_assert(num_spikes <= max_num_packed, "Pack size too large, is not supported.");

	/** Payload of a SpikePack instruction. */
	class Payload
	{
	public:
		typedef hate::bitset<size> value_type;
		typedef std::array<Spike, num_spikes> spikes_type;

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
				ret |= value_type(m_spikes[i - 1].encode()) << ((num_spikes - i) * Spike::size);
			}
			return ret;
		}

		template <class SubwordType = unsigned long>
		void decode(hate::bitset<size, SubwordType> const& data)
		{
			for (size_t i = 1; i <= num_spikes; ++i) {
				m_spikes[i - 1].decode(
				    typename Spike::value_type(data >> ((num_spikes - i) * Spike::size)));
			}
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
typedef
    typename detail::GenerateDictionary<SpikePack, std::make_index_sequence<max_num_packed>>::type
        Dictionary;

} // namespace hxcomm::vx::instruction::event_to_fpga
