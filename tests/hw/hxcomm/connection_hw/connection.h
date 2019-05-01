#pragma once
#include "hxcomm/vx/arqconnection.h"

#define HXCOMM_TEST_ARQ_CONNECTION
typedef hxcomm::vx::ARQConnection TestConnection;

TestConnection generate_test_connection();
