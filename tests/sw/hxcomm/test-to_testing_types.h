#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hxcomm/vx/utmessage.h"

using namespace hxcomm::vx;
using namespace hxcomm::vx::instruction;

template <typename ToTL, typename FromTL>
struct to_testing_types;

template <typename... ToIs, typename... FromIs>
struct to_testing_types<hate::type_list<ToIs...>, hate::type_list<FromIs...>>
{
	typedef ::testing::Types<UTMessageToFPGA<ToIs>..., UTMessageFromFPGA<FromIs>...> type;
};
