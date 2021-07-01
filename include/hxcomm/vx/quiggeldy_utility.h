#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/quiggeldy_utility.h"

#include "hxcomm/vx/quiggeldy_server.h"
#include "hxcomm/vx/quiggeldy_worker.h"

#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/simconnection.h"

#include "rcf-extensions/round-robin-reinit-scheduler.h"

namespace hxcomm::vx {

/**
 * Launch a local quiggeldy without slurm allocation and debug output based on
 * current env settings. The environment will be modified so that default
 * constructed QuiggeldyConnection-instances automatically connect to it.
 *
 * The release idle timeout is set to a day so that for all testing purposes a
 * single connection will be used.
 *
 * @return Tuple of process ID and port of launched quiggeldy process and the
 * port it is listening on.
 */
std::tuple<pid_t, hxcomm::port_t> launch_quiggeldy_locally_from_env() SYMBOL_VISIBLE;

RRWR_GENERATE_UTILITIES(
    ::hxcomm::vx::QuiggeldyWorker<ARQConnection>, quiggeldy_arq, ::hxcomm::vx::I_HXCommQuiggeldyVX)

RRWR_GENERATE_UTILITIES(
    ::hxcomm::vx::QuiggeldyWorker<SimConnection>, quiggeldy_sim, ::hxcomm::vx::I_HXCommQuiggeldyVX)

} // namespace hxcomm::vx
