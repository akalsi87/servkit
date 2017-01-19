/**
 * \file likely.h
 * \date Jan 18, 2017
 */

#ifndef _SERVKIT_LIKELY_H_
#define _SERVKIT_LIKELY_H_

#define SK_LIKELY(x) __builtin_expect(!!(x),1)
#define SK_UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif/*_SERVKIT_LIKELY_H_*/
