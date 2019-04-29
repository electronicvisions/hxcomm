#pragma once
#include <climits>

#include "halco/hicann-dls/vx/coordinates.h"
#include "hxcomm/common/payload.h"

/** Instructions for events from the FPGA. */
namespace hxcomm::vx::instruction::event_from_fpga {

constexpr size_t max_num_packed = 3;

/** Type of one sample event. */
class MADCSample
{
public:
	constexpr static size_t value_size = 16;
	constexpr static size_t timestamp_size = 8;
	constexpr static size_t size = value_size + timestamp_size;

	typedef hate::bitset<size> value_type;

	struct Value : public halco::common::detail::RantWrapper<Value, uint16_t, 0x3fff, 0>
	{
		typedef halco::common::detail::RantWrapper<Value, uint16_t, 0x3fff, 0> rant_t;
		explicit Value(uintmax_t const value = 0) : rant_t(value) {}
	};

	struct Timestamp : public halco::common::detail::RantWrapper<Timestamp, uint16_t, 0xff, 0>
	{
		typedef halco::common::detail::RantWrapper<Timestamp, uint16_t, 0xff, 0> rant_t;
		explicit Timestamp(uintmax_t const value = 0) : rant_t(value) {}
	};

	MADCSample() : m_value(), m_timestamp() {}

	MADCSample(Value const& value, Timestamp const& timestamp) :
	    m_value(value),
	    m_timestamp(timestamp)
	{}

	Value const& get_value() const { return m_value; }
	void set_value(Value const& value) { m_value = value; }

	Timestamp const& get_timestamp() const { return m_timestamp; }
	void set_timestamp(Timestamp const& timestamp) { m_timestamp = timestamp; }

	bool operator==(MADCSample const& other) const
	{
		return (m_value == other.m_value) && m_timestamp == other.m_timestamp;
	}

	bool operator!=(MADCSample const& other) const { return !(*this == other); }

	template <class SubwordType = unsigned long>
	hate::bitset<size, SubwordType> encode() const
	{
		return (value_type(m_timestamp) << value_size) | value_type(m_value);
	}

	template <class SubwordType = unsigned long>
	void decode(hate::bitset<size, SubwordType> const& data)
	{
		m_timestamp = Timestamp(static_cast<uintmax_t>(data >> value_size));
		m_value = Value(static_cast<uintmax_t>(hate::bitset<14, SubwordType>(data)));
	}

private:
	Value m_value;
	Timestamp m_timestamp;
};


template <size_t num_samples>
struct MADCSamplePack
{
	constexpr static size_t size = MADCSample::size * num_samples;

	static_assert(num_samples <= max_num_packed, "Pack size too large, is not supported.");

	/** Payload of a madc_sample_pack instruction. */
	class Payload
	{
	public:
		typedef hate::bitset<size> value_type;

		typedef std::array<MADCSample, num_samples> samples_type;

		Payload() : m_samples() {}
		Payload(samples_type const& samples) : m_samples(samples) {}

		samples_type const& get_samples() const { return m_samples; }
		void set_samples(samples_type const& samples) { m_samples = samples; }

		bool operator==(Payload const& other) const { return m_samples == other.m_samples; }
		bool operator!=(Payload const& other) const { return !(*this == other); }

		template <class SubwordType = unsigned long>
		hate::bitset<size, SubwordType> encode() const
		{
			value_type ret;
			for (size_t i = 1; i <= num_samples; ++i) {
				ret |= value_type(m_samples[i - 1].encode())
				       << ((num_samples - i) * MADCSample::size);
			}
			return ret;
		}

		template <class SubwordType = unsigned long>
		void decode(hate::bitset<size, SubwordType> const& data)
		{
			for (size_t i = 1; i <= num_samples; ++i) {
				m_samples[i - 1].decode(typename MADCSample::value_type(
				    data >> ((num_samples - i) * MADCSample::size)));
			}
		}

	private:
		samples_type m_samples;
	};
};


/** Type of one spike event. */
class Spike
{
public:
	constexpr static size_t neuron_address_size = 14;
	constexpr static size_t spl1_address_size = 2;
	constexpr static size_t timestamp_size = 8;
	constexpr static size_t size = neuron_address_size + spl1_address_size + timestamp_size;

	typedef hate::bitset<size> value_type;

	typedef halco::hicann_dls::vx::NeuronLabel neuron_label_type;
	typedef halco::hicann_dls::vx::SPL1Address spl1_address_type;

	struct Timestamp : public halco::common::detail::RantWrapper<Timestamp, uint16_t, 0xff, 0>
	{
		typedef halco::common::detail::RantWrapper<Timestamp, uint16_t, 0xff, 0> rant_t;
		explicit Timestamp(uintmax_t const value = 0) : rant_t(value) {}
	};

	Spike() : m_neuron(), m_spl1(), m_timestamp() {}
	Spike(
	    neuron_label_type const& neuron,
	    spl1_address_type const& spl1,
	    Timestamp const& timestamp) :
	    m_neuron(neuron),
	    m_spl1(spl1),
	    m_timestamp(timestamp)
	{}

	neuron_label_type const& get_neuron_label() const { return m_neuron; }
	void set_neuron_label(neuron_label_type const& neuron) { m_neuron = neuron; }

	spl1_address_type const& get_spl1_address() const { return m_spl1; }
	void set_spl1_address(spl1_address_type const& spl1) { m_spl1 = spl1; }

	Timestamp const& get_timestamp() const { return m_timestamp; }
	void set_timestamp(Timestamp const& timestamp) { m_timestamp = timestamp; }

	bool operator==(Spike const& other) const
	{
		return (m_neuron == other.m_neuron) && (m_spl1 == other.m_spl1) &&
		       (m_timestamp == other.m_timestamp);
	}

	bool operator!=(Spike const& other) const { return !(*this == other); }

	template <class SubwordType = unsigned long>
	hate::bitset<size, SubwordType> encode() const
	{
		return (value_type(m_timestamp) << (neuron_address_size + spl1_address_size)) |
		       (value_type(m_spl1) << neuron_address_size) | value_type(m_neuron);
	}

	template <class SubwordType = unsigned long>
	void decode(hate::bitset<size, SubwordType> const& data)
	{
		m_timestamp =
		    Timestamp(static_cast<uintmax_t>(data >> (neuron_address_size + spl1_address_size)));
		m_spl1 = spl1_address_type(
		    static_cast<uintmax_t>(hate::bitset<spl1_address_size>(data >> neuron_address_size)));
		m_neuron = neuron_label_type(
		    static_cast<uintmax_t>(hate::bitset<neuron_address_size, SubwordType>(data)));
	}

private:
	neuron_label_type m_neuron;
	spl1_address_type m_spl1;
	Timestamp m_timestamp;
};


template <size_t num_spikes>
struct SpikePack
{
	constexpr static size_t size = Spike::size * num_spikes;

	static_assert(num_spikes <= max_num_packed, "Pack size too large, is not supported.");

	/** Payload of a spike_pack instruction. */
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

template <template <size_t> class PackSpike, template <size_t> class PackMADC, typename IS>
struct GenerateDictionary;

template <template <size_t> class PackSpike, template <size_t> class PackMADC, size_t... Is>
struct GenerateDictionary<PackSpike, PackMADC, std::index_sequence<Is...>>
{
	typedef hate::type_list<PackMADC<Is + 1>..., PackSpike<Is + 1>...> type;
};

} // namespace detail

/** Dictionary of all events from FPGA instructions. */
typedef typename detail::
    GenerateDictionary<SpikePack, MADCSamplePack, std::make_index_sequence<max_num_packed>>::type
        Dictionary;

} // namespace hxcomm::vx::instruction::event_from_fpga
