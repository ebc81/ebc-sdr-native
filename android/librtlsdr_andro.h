/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
 *
 * Modification 2013 by Martin Marinov <martintzvetomirov@gmail.com>
 * Modifications: opening a device via file descriptor
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

#include "rtl-sdr.h"

#ifndef __RTL_ANDRO_SDR_H
#define __RTL_ANDRO_SDR_H

/*
 * __EBCANDROID__: error codes of this library. Vorarbeit 4.3 of
 * KONZEPT-GEMEINSAME-CODEBASE.md.
 *
 * rtlsdr_open2() returns 0 on success, one of these on a failure it recognises, or a raw
 * negative libusb code (LIBUSB_ERROR_*, -1 .. -12 and -99) when libusb itself refused.
 *
 * The -2000 block is deliberately far away from everything already in play, so a caller
 * can tell by range alone where a code came from:
 *
 *   -1 .. -12, -99   libusb          LIBUSB_ERROR_*
 *   -50 .. -55       RTL_SDR_AIS_Driver   EXIT_TEST_RTLSDR_OPEN1..6 (rtlaisjava_err.h)
 *   -101, -102       rtlsdr433 / rtlsdrPager  their local EXIT_TEST_RTLSDR_OPEN1/2
 *   -2001 ...        this library
 *
 * Each app keeps its own numbering at its JNI boundary -- the Java contract of
 * RTL_SDR_AIS_Driver in particular must not change. Map at the boundary, do not renumber
 * here:
 *
 *   EBC_SDR_ERR_LIBUSB_INIT  -> EXIT_TEST_RTLSDR_OPEN1  (-50 in AIS, -101 in 433/Pager)
 *   EBC_SDR_ERR_CLAIM        -> EXIT_TEST_RTLSDR_OPEN2  (-51 in AIS, -102 in 433/Pager)
 *
 * For EBC_SDR_ERR_CLAIM, ask rtlsdr_last_open_was_busy() before wording the message: a
 * busy interface right after a re-plug means "wait a moment", not "cannot open".
 */
#define EBC_SDR_ERR_LIBUSB_INIT   -2001  /* libusb_init() gave no context */
#define EBC_SDR_ERR_CLAIM         -2002  /* libusb_claim_interface() failed */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Open a device from a file descriptor handed over by the Java/Kotlin USB layer.
 *
 * Returns 0 and a device in *out_dev, or one of the EBC_SDR_ERR_* codes above, or a raw
 * negative LIBUSB_ERROR_*. The caller owns the device and closes it with rtlsdr_close().
 */
RTLSDR_API int rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd);
RTLSDR_API int rtlsdr_cancel_async_save(rtlsdr_dev_t *dev);
RTLSDR_API int rtlsdr_cancel_async_save_fast(rtlsdr_dev_t *dev);

/* Backported from rtlsdrPager; see librtlsdr_andro.c for why each one exists. */
RTLSDR_API int rtlsdr_last_open_was_busy(void);
RTLSDR_API int rtlsdr_is_dev_lost(rtlsdr_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __RTL_ANDRO_SDR_H */
