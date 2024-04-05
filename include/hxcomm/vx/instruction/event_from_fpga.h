#pragma once
#include "hate/join.h"
#include "hate/type_index.h"
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include "hxcomm/vx/instruction/event_constants.h"
#include <climits>

/** Instructions for events from the FPGA. */
namespace hxcomm::vx::instruction::event_from_fpga {

/** Type of one sample event. */
class MADCSample
{
public:
	constexpr static size_t value_size = 16;
	constexpr static size_t timestamp_size = 8;
	constexpr static size_t size = value_size + timestamp_size;

	typedef hate::bitset<size> value_type;

	typedef hate::bitset<14> Value;
	typedef hate::bitset<timestamp_size> Timestamp;

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

	friend std::ostream& operator<<(std::ostream& os, MADCSample const& value)
	{
		os << "MADCSample(" << value.m_value << ", " << value.m_timestamp << ")";
		return os;
	}

private:
	Value m_value;
	Timestamp m_timestamp;
};


template <size_t num_samples>
struct MADCSamplePack;


/** Payload of a madc_sample_pack instruction. */
template <size_t num_samples>
class MADCSamplePackPayload
{
public:
	constexpr static size_t size = MADCSample::size * num_samples;

	typedef hate::bitset<size> value_type;

	typedef std::array<MADCSample, num_samples> samples_type;

	MADCSamplePackPayload() : m_samples() {}
	MADCSamplePackPayload(samples_type const& samples) : m_samples(samples) {}

	samples_type const& get_samples() const { return m_samples; }
	void set_samples(samples_type const& samples) { m_samples = samples; }

	bool operator==(MADCSamplePackPayload const& other) const
	{
		return m_samples == other.m_samples;
	}
	bool operator!=(MADCSamplePackPayload const& other) const { return !(*this == other); }

	template <class SubwordType = unsigned long>
	hate::bitset<size, SubwordType> encode() const
	{
		value_type ret;
		for (size_t i = 1; i <= num_samples; ++i) {
			ret |= value_type(m_samples[i - 1].encode()) << ((num_samples - i) * MADCSample::size);
		}
		return ret;
	}

	template <class SubwordType = unsigned long>
	void decode(hate::bitset<size, SubwordType> const& data)
	{
		for (size_t i = 1; i <= num_samples; ++i) {
			m_samples[i - 1].decode(
			    typename MADCSample::value_type(data >> ((num_samples - i) * MADCSample::size)));
		}
	}

	friend std::ostream& operator<<(std::ostream& os, MADCSamplePackPayload const& value)
	{
		os << hate::full_name<MADCSamplePack<num_samples>>() << "("
		   << hate::join(value.m_samples, ", ") << ")";
		return os;
	}

private:
	samples_type m_samples;
};


template <size_t num_samples>
struct MADCSamplePack
{
	constexpr static size_t size = MADCSamplePackPayload<num_samples>::size;

	static_assert(
	    num_samples <= event_constants::max_num_packed, "Pack size too large, is not supported.");

	typedef MADCSamplePackPayload<num_samples> Payload;
};


/** Type of one spike event. */
class Spike
{
public:
	constexpr static size_t timestamp_size = 8;
	constexpr static size_t size = event_constants::spike_size + timestamp_size;

	typedef hate::bitset<size> value_type;

	typedef hate::bitset<event_constants::spike_size> spike_type;

	typedef hate::bitset<timestamp_size> Timestamp;

	Spike() : m_spike(), m_timestamp() {}
	Spike(spike_type const& spike, Timestamp const& timestamp) :
	    m_spike(spike),
	    m_timestamp(timestamp)
	{}

	spike_type const& get_spike() const { return m_spike; }
	void set_spike(spike_type const& spike) { m_spike = spike; }

	Timestamp const& get_timestamp() const { return m_timestamp; }
	void set_timestamp(Timestamp const& timestamp) { m_timestamp = timestamp; }

	bool operator==(Spike const& other) const
	{
		return (m_spike == other.m_spike) && (m_timestamp == other.m_timestamp);
	}

	bool operator!=(Spike const& other) const { return !(*this == other); }

	template <class SubwordType = unsigned long>
	hate::bitset<size, SubwordType> encode() const
	{
		return (value_type(m_timestamp) << event_constants::spike_size) | value_type(m_spike);
	}

	template <class SubwordType = unsigned long>
	void decode(hate::bitset<size, SubwordType> const& data)
	{
		m_timestamp = Timestamp(static_cast<uintmax_t>(data >> event_constants::spike_size));
		m_spike = spike_type(data);
	}

	friend std::ostream& operator<<(std::ostream& os, Spike const& value)
	{
		os << "Spike(" << value.m_spike << ", " << value.m_timestamp << ")";
		return os;
	}

private:
	spike_type m_spike;
	Timestamp m_timestamp;
};


template <size_t num_spikes>
struct SpikePack;


/** Payload of a spike_pack instruction. */
template <size_t num_spikes>
class SpikePackPayload
{
public:
	constexpr static size_t size = Spike::size * num_spikes;

	typedef hate::bitset<size> value_type;
	typedef std::array<Spike, num_spikes> spikes_type;

	SpikePackPayload() : m_spikes() {}
	SpikePackPayload(spikes_type const& spikes) : m_spikes(spikes) {}

	spikes_type const& get_spikes() const { return m_spikes; }
	void set_spikes(spikes_type const& spikes) { m_spikes = spikes; }

	bool operator==(SpikePackPayload const& other) const { return m_spikes == other.m_spikes; }
	bool operator!=(SpikePackPayload const& other) const { return !(*this == other); }

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

	friend std::ostream& operator<<(std::ostream& os, SpikePackPayload const& value)
	{
		os << hate::full_name<SpikePack<num_spikes>>() << "(" << hate::join(value.m_spikes, ", ")
		   << ")";
		return os;
	}

private:
	spikes_type m_spikes;
};


template <size_t num_spikes>
struct SpikePack
{
	constexpr static size_t size = SpikePackPayload<num_spikes>::size;

	static_assert(
	    num_spikes <= event_constants::max_num_packed, "Pack size too large, is not supported.");

	typedef SpikePackPayload<num_spikes> Payload;
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
typedef typename detail::GenerateDictionary<
    SpikePack,
    MADCSamplePack,
    std::make_index_sequence<event_constants::max_num_packed>>::type Dictionary;

} // namespace hxcomm::vx::instruction::event_from_fpga
