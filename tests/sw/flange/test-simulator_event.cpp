#include <gtest/gtest.h>

#include "SF/ITextStream.hpp"
#include "SF/OTextStream.hpp"
#include "flange/simulator_event.h"

using namespace flange;

TEST(SimulatorEvent, General)
{
	EXPECT_NO_THROW(SimulatorEvent());

	auto default_event = SimulatorEvent();
	EXPECT_EQ(default_event.event_type, SimulatorEvent::TERMINATE);
	EXPECT_EQ(default_event.data, 0);
	EXPECT_EQ(default_event.timestamp, 0);

	EXPECT_NO_THROW(SimulatorEvent(SimulatorEvent::PAUSE, 17));

	EXPECT_NO_THROW(SimulatorEvent(SimulatorEvent::PAUSE_AFTER_RECEIVE, 17));

	// only event with event_type AL_DATA is allowed to be constructed from non-zero data
	EXPECT_THROW(SimulatorEvent(SimulatorEvent::PAUSE, 17, 23), std::runtime_error);
	EXPECT_THROW(SimulatorEvent(SimulatorEvent::PAUSE_AFTER_RECEIVE, 17, 23), std::runtime_error);
	EXPECT_THROW(SimulatorEvent(SimulatorEvent::TERMINATE, 17, 23), std::runtime_error);

	SimulatorEvent event1(SimulatorEvent::AL_DATA, 1, 17);
	SimulatorEvent event2 = event1;

	EXPECT_TRUE(event1 == event2);
	EXPECT_FALSE(event1 != event2);

	event2.event_type = SimulatorEvent::PAUSE;
	EXPECT_TRUE(event1 != event2);
	EXPECT_FALSE(event1 == event2);

	auto event3 = event1;
	event3.timestamp = 2;
	EXPECT_TRUE(event1 != event3);
	EXPECT_FALSE(event1 == event3);

	auto event4 = event1;
	event4.data = 2;
	EXPECT_TRUE(event1 != event4);
	EXPECT_FALSE(event1 == event4);
}

TEST(SimulatorEvent, Serialization)
{
	SimulatorEvent event(SimulatorEvent::AL_DATA, 17, 23);

	std::stringstream ss;
	{
		SF::OTextStream os(ss);
		SF::Archive ar(SF::Archive::Direction::WRITE, &os);
		event.serialize(ar);
	}

	SimulatorEvent event_deserialized;
	{
		SF::ITextStream is(ss);
		SF::Archive ar(SF::Archive::Direction::READ, &is);
		event_deserialized.serialize(ar);
	}

	EXPECT_EQ(event, event_deserialized);
}
