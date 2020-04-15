// No pragma once because header could be included several times with different
// macro settings!

#ifndef QUIGGELDY_REQUEST_T
#error "Please include 'hxcomm/<architecture>/quiggeldy_rcf.h' instead of this file!"
#endif

#ifndef QUIGGELDY_RESPONSE_T
#error "Please include 'hxcomm/<architecture>/quiggeldy_rcf.h' instead of this file!"
#endif

#ifndef QUIGGELDY_RCF_INTERFACE
#error "Please include 'hxcomm/<architecture>/quiggeldy_rcf.h' instead of this file!"
#endif

#ifdef USE_MUNGE_AUTH
#include <munge.h>
#endif

#ifndef __GENPYBIND__

// Filter "known" warnings from RCF that we cannot do anything about.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wterminate"

#include <RCF/RCF.hpp>

namespace SF {

class Archive;

inline void serialize(Archive& ar, QUIGGELDY_REQUEST_T& qcr);
inline void serialize(Archive& ar, QUIGGELDY_RESPONSE_T& qcr);

} // namespace SF

#endif // genpybind

namespace hxcomm {

#ifndef __GENPYBIND__

#ifdef QUIGGELDY_MAKE_RCF_INTERFACE
#error "QUIGGELDY_MAKE_RCF_INTERFACE already defined!"
#endif

// macro needed because we want to expand the defined interface macro both as macro parameter and
// string literal
#define QUIGGELDY_MAKE_RCF_INTERFACE(INTERFACE)                                                    \
	RCF_BEGIN(INTERFACE, #INTERFACE)                                                               \
	RCF_METHOD_R1(QUIGGELDY_RESPONSE_T, submit_work, QUIGGELDY_REQUEST_T const&)                   \
	RCF_END(INTERFACE)

QUIGGELDY_MAKE_RCF_INTERFACE(QUIGGELDY_RCF_INTERFACE)

#pragma GCC diagnostic pop

#undef QUIGGELDY_MAKE_RCF_INTERFACE

#else

// empty definition for interface
class QUIGGELDY_RCF_INTERFACE
{};

#endif // genpybind

} // namespace hxcomm

#ifndef __GENPYBIND__
#include "hxcomm/common/quiggeldy_rcf.tcc"
#endif
