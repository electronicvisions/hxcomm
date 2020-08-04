#pragma once
#include "hxcomm/vx/utmessage.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

template <typename ToTL, typename FromTL, typename... Others>
struct to_testing_types;

template <typename... ToIs, typename... FromIs, typename... Others>
struct to_testing_types<hate::type_list<ToIs...>, hate::type_list<FromIs...>, Others...>
{
	typedef ::testing::Types<UTMessageToFPGA<ToIs>..., UTMessageFromFPGA<FromIs>..., Others...>
	    type;
};
