/**
 * \file asserts.h
 * \date Jan 18, 2017
 */

#ifndef _SERVKIT_ASSERTS_H_
#define _SERVKIT_ASSERTS_H_

int _skAssert(char const* msg, char const* file, int line);

#ifndef NDEBUG
#define skAssert(expr) ((void)(!!(expr) ? 0 : _skAssert(#expr, __FILE__, __LINE__)))
#define skAssertMsg(expr,msg) ((void)(!!(expr) ? 0 : _skAssert((msg), __FILE__, __LINE__)))
#else
#define skAssert(expr) ((void)0)
#define skAssertMsg(expr,msg) ((void)0)
#endif

#endif/*_SERVKIT_ASSERTS_H_*/
