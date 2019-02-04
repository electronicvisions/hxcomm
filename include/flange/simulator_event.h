#pragma once

#include <cstdint>

namespace SF {
class Archive;
} // namespace SF

namespace flange {

/**
 * Event to be sent to simulation.
 */
struct SimulatorEvent
{
	/** Time type in cycles */
	typedef uint64_t clk_t;
	/** Payload data type */
	typedef uint64_t al_data_t;

	/** Event type */
	enum event_types
	{
		TERMINATE,
		AL_DATA,
		PAUSE,
		PAUSE_AFTER_RECEIVE
	};

	/** Default constructor. */
	SimulatorEvent();

	/** Constructor validating that (data != 0) only if (event_type == AL_DATA). */
	SimulatorEvent(event_types event_type, clk_t timestamp, al_data_t data = 0);

	/** Type of event. */
	event_types event_type;

	/** Timestamp of event. */
	clk_t timestamp;
	/** Timestamp value encoding (also) "ASAP" timing. */
	static const clk_t timestamp_asap;

	/** Data field of event. Only valid if event_type == AL_DATA */
	al_data_t data;

	bool operator==(SimulatorEvent const& other) const;
	bool operator!=(SimulatorEvent const& other) const;

	/** SF serialization used for RCF communication. */
	void serialize(SF::Archive& ar);
};

} // namespace flange
