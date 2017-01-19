/**
 * \file asserts.h
 * \date Jan 18, 2017
 */

#ifndef _SERVKIT_ASSERTS_H_
#define _SERVKIT_ASSERTS_H_

#include <servkit/exportsym.h>
#include <stdio.h>

SK_API int _skAssert(char const* msg, char const* file, int line);

SK_API int _skTracePrintFileLine(char const* lvl, char const* file, int line);

SK_API
/*!
 * Get the trace file.
 */
FILE* skGetTraceFile(void);

SK_API
/*!
 * Set the trace file.
 */
void skSetTraceFile(FILE* file);

/* Asserts */
#ifndef NDEBUG
#define skAssert(expr) ((void)(!!(expr) ? 0 : _skAssert(#expr, __FILE__, __LINE__)))
#define skAssertMsg(expr,msg) ((void)(!!(expr) ? 0 : _skAssert((msg), __FILE__, __LINE__)))
#else
#define skAssert(expr) ((void)0)
#define skAssertMsg(expr,msg) ((void)0)
#endif

/*! Trace levels */
/*! Can define SK_TRACE_NO_COLOR to disable colors from trace outputs */
#if !defined(_WIN32) || !defined(SK_TRACE_NO_COLOR)
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#else
#define ANSI_COLOR_RED
#define ANSI_COLOR_GREEN
#define ANSI_COLOR_YELLOW
#define ANSI_COLOR_BLUE
#define ANSI_COLOR_MAGENTA
#define ANSI_COLOR_CYAN
#define ANSI_COLOR_RESET
#endif

#define SK_INFO ANSI_COLOR_RESET  "[INFO] "
#define SK_WARN ANSI_COLOR_YELLOW "[WARN] "
#define SK_ERR  ANSI_COLOR_RED    "[ERROR] "
#define SK_SUCC ANSI_COLOR_GREEN  "[OK] "

/*! Tracer */
#define skTrace(lvl,fmt,...) fprintf(skGetTraceFile(), lvl ANSI_COLOR_RESET fmt "\n", ## __VA_ARGS__)
#define skTraceF(lvl,fmt,...) _skTracePrintFileLine(lvl, __FILE__, __LINE__); fprintf(skGetTraceFile(), " " ANSI_COLOR_RESET fmt "\n", ## __VA_ARGS__)

#endif/*_SERVKIT_ASSERTS_H_*/
