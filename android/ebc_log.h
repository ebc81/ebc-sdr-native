/*
 * Logging for the shared EBC SDR native base.
 *
 * Copyright (C) 2026 Christian Ebner
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * __EBCANDROID__: not upstream. Vorarbeit 4.3 of KONZEPT-GEMEINSAME-CODEBASE.md.
 *
 * Two jobs.
 *
 * 1. `aprintf_stderr()` for the Android bridge.
 *    It was a *function* declared in RTL_SDR_AIS_Driver's rtl_ais_andro.h and a *macro* over
 *    __android_log_print in rtlsdr433/rtlsdrPager. The bridge had to pick one, and picking the
 *    function meant the shared tree could not compile without an app header. It is a macro
 *    here, with the log tag configurable.
 *
 * 2. Befund 10 of ANALYSE.md: rtl-sdr writes all its diagnostics with
 *    `fprintf(stderr, ...)`, and on Android stderr goes nowhere. 83 calls across librtlsdr.c,
 *    convenience.c and the four tuner_*.c -- tuner detection, PLL failures, the exact sample
 *    rate, "No supported tuner found". All of it invisible while debugging, which is why
 *    Phase 0 could only remove the unused <android/log.h> include and leave the rest.
 *
 *    `fprintf` is redirected here rather than at the 83 call sites, so the vendored sources
 *    stay diffable against upstream. The redirect is deliberately *not* blind: the shim looks
 *    at the stream, so a genuine `fprintf` to a file still goes to that file. Only stderr and
 *    stdout are diverted into logcat.
 *
 * Configure the tag from CMake: `-DEBC_LOG_TAG="\"pager\""`. Default below.
 *
 * Everything is guarded by __ANDROID__ so the tree still builds on a host for analysis.
 */

#ifndef __EBC_LOG_H
#define __EBC_LOG_H

#include <stdio.h>

#ifndef EBC_LOG_TAG
#define EBC_LOG_TAG "librtlsdr"
#endif

#if defined(__ANDROID__)

#include <stdarg.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Route a stdio stream to logcat, or to the stream itself if it is a real file.
 *
 * stderr -> WARN, because that is what rtl-sdr uses for both errors and progress notes;
 *           calling all of it ERROR would cry wolf on "Exact sample rate is: ..."
 * stdout -> INFO
 */
/* format(printf, 2, 0) marks this as vprintf-style, so forwarding our own non-literal
 * fmt on to __android_log_vprint/vfprintf does not trip -Wformat-nonliteral. */
static inline int ebc_log_vfprintf(FILE *stream, const char *fmt, va_list ap)
	__attribute__((format(printf, 2, 0)));

static inline int ebc_log_vfprintf(FILE *stream, const char *fmt, va_list ap)
{
	if (stream == stderr)
		return __android_log_vprint(ANDROID_LOG_WARN, EBC_LOG_TAG, fmt, ap);
	if (stream == stdout)
		return __android_log_vprint(ANDROID_LOG_INFO, EBC_LOG_TAG, fmt, ap);
	return vfprintf(stream, fmt, ap);
}

/*
 * The format attribute is not decoration: without it the `fprintf` macro below would silence
 * -Wformat on all 83 vendored call sites, and a wrong conversion specifier in vendored code
 * is exactly the kind of thing this tree must keep catching.
 */
static inline int ebc_log_fprintf(FILE *stream, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

static inline int ebc_log_fprintf(FILE *stream, const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = ebc_log_vfprintf(stream, fmt, ap);
	va_end(ap);

	return n;
}

#ifdef __cplusplus
}
#endif

/*
 * Redirect the vendored diagnostics. Parenthesise the name -- `(fprintf)(...)` -- to call the
 * real one if you ever need to.
 */
#define fprintf ebc_log_fprintf

/* The bridge's own error channel. */
#define aprintf_stderr(...) __android_log_print(ANDROID_LOG_ERROR, EBC_LOG_TAG, __VA_ARGS__)
#define aprintf_info(...)   __android_log_print(ANDROID_LOG_INFO,  EBC_LOG_TAG, __VA_ARGS__)

#else /* not Android: leave stdio alone */

#define aprintf_stderr(...) fprintf(stderr, __VA_ARGS__)
#define aprintf_info(...)   fprintf(stderr, __VA_ARGS__)

#endif /* __ANDROID__ */

#endif /* __EBC_LOG_H */
