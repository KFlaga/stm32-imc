#pragma once

#if (defined(DEBUG) || defined(_DEBUG)) && !defined(NDEBUG) && !defined(NO_ASSERT)
extern void dyna_assert_impl(bool cond, const char* expr, const char* file, int line);
#define dyna_assert(cond) dyna_assert_impl(cond, #cond, __FILE__, __LINE__)
#else
#define dyna_assert(cond)
#endif
