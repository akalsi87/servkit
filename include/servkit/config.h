/**
 * \file config.h
 * \date Jan 18, 2017
 */

#ifndef _SERVKIT_CONFIG_H_
#define _SERVKIT_CONFIG_H_

#define _BSD_SOURCE

#if defined(__linux__)
#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#endif

#if defined(_AIX)
#define _ALL_SOURCE
#endif

#if defined(__linux__) || defined(__OpenBSD__)
#define _XOPEN_SOURCE 700
/*
 * On NetBSD, _XOPEN_SOURCE undefines _NETBSD_SOURCE and
 * thus hides inet_aton etc.
 */
#elif !defined(__NetBSD__)
#define _XOPEN_SOURCE
#endif

#if defined(__sun)
#define _POSIX_C_SOURCE 199506L
#endif

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <servkit/exportsym.h>

typedef unsigned long long sk_ull;
typedef long long sk_ll;

#endif/*_SERVKIT_CONFIG_H_*/
