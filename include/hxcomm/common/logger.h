#pragma once
#include <log4cxx/logger.h>

#if !defined(HXCOMM_LOG_THRESHOLD) || HXCOMM_LOG_THRESHOLD <= 0
#define HXCOMM_LOG_TRACE(logger, message) LOG4CXX_TRACE(logger, message)
#else
#define HXCOMM_LOG_TRACE(logger, message)
#endif

#if !defined(HXCOMM_LOG_THRESHOLD) || HXCOMM_LOG_THRESHOLD <= 1
#define HXCOMM_LOG_DEBUG(logger, message) LOG4CXX_DEBUG(logger, message)
#else
#define HXCOMM_LOG_DEBUG(logger, message)
#endif

#if !defined(HXCOMM_LOG_THRESHOLD) || HXCOMM_LOG_THRESHOLD <= 2
#define HXCOMM_LOG_INFO(logger, message) LOG4CXX_INFO(logger, message)
#else
#define HXCOMM_LOG_INFO(logger, message)
#endif

#if !defined(HXCOMM_LOG_THRESHOLD) || HXCOMM_LOG_THRESHOLD <= 3
#define HXCOMM_LOG_WARN(logger, message) LOG4CXX_WARN(logger, message)
#else
#define HXCOMM_LOG_WARN(logger, message)
#endif

#if !defined(HXCOMM_LOG_THRESHOLD) || HXCOMM_LOG_THRESHOLD <= 4
#define HXCOMM_LOG_ERROR(logger, message) LOG4CXX_ERROR(logger, message)
#else
#define HXCOMM_LOG_ERROR(logger, message)
#endif

#if !defined(HXCOMM_LOG_THRESHOLD) || HXCOMM_LOG_THRESHOLD <= 5
#define HXCOMM_LOG_FATAL(logger, message) LOG4CXX_FATAL(logger, message)
#else
#define HXCOMM_LOG_FATAL(logger, message)
#endif
