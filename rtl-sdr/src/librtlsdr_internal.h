/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
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
 * __EBCANDROID__: internal interface of librtlsdr.c. Not upstream.
 *
 * Why this file exists
 * --------------------
 * On Android there is no device enumeration, so a device can only be opened from a file
 * descriptor handed over by the Java/Kotlin layer. `rtlsdr_open2()` in
 * android/librtlsdr_andro.c therefore builds the private `struct rtlsdr_dev` itself instead
 * of going through `rtlsdr_open()`, and to do that it needs the struct definition and a
 * handful of register-level helpers.
 *
 * Until Phase 0 it got them the crude way: `#include "rtl-sdr/src/librtlsdr.c"` inline. That
 * cost two things -- librtlsdr.c could never be a translation unit of its own (listing it in
 * CMake alongside the bridge produced duplicate symbols), and the relative directory layout
 * of the two trees was frozen. Both blocked turning this into a library.
 *
 * So this header carries what the bridge needs and nothing more. The contents were **moved
 * verbatim** out of librtlsdr.c, not rewritten -- see PROVENANCE.md. librtlsdr.c is now a
 * normal translation unit and the bridge includes only this header.
 *
 * This is not a public API. Nothing outside librtlsdr.c and android/librtlsdr_andro.c may
 * include it: `struct rtlsdr_dev` is private on purpose, and its layout changes without
 * notice. Applications use rtl-sdr.h and librtlsdr_andro.h.
 */

#ifndef __LIBRTLSDR_INTERNAL_H
#define __LIBRTLSDR_INTERNAL_H

#include <stdint.h>

#include <libusb.h>

#include "rtl-sdr.h"
#include "tuner_e4k.h"
#include "tuner_fc0012.h"
#include "tuner_fc0013.h"
#include "tuner_fc2580.h"
#include "tuner_r82xx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtlsdr_tuner_iface {
	/* tuner interface */
	int (*init)(void *);
	int (*exit)(void *);
	int (*set_freq)(void *, uint32_t freq /* Hz */);
	int (*set_bw)(void *, int bw /* Hz */);
	int (*set_gain)(void *, int gain /* tenth dB */);
	int (*set_if_gain)(void *, int stage, int gain /* tenth dB */);
	int (*set_gain_mode)(void *, int manual);
} rtlsdr_tuner_iface_t;

enum rtlsdr_async_status {
	RTLSDR_INACTIVE = 0,
	RTLSDR_CANCELING,
	RTLSDR_RUNNING
};

#define FIR_LEN 16
#define EEPROM_SIZE	256
#define STR_OFFSET	0x09

/* Reference crystal of the RTL2832. The bridge seeds dev->rtl_xtal with it. */
#define DEF_RTL_XTAL_FREQ	28800000
#define MIN_RTL_XTAL_FREQ	(DEF_RTL_XTAL_FREQ - 1000)
#define MAX_RTL_XTAL_FREQ	(DEF_RTL_XTAL_FREQ + 1000)

/*
 * RTL2832 register map. The bridge needs USBB and USB_SYSCTL for the dummy write that
 * decides whether the device has to be reset before use; the rest of both enums comes
 * along because an enumerator cannot be moved on its own, and because a register map
 * belongs in a header rather than in one translation unit.
 */
enum usb_reg {
	USB_SYSCTL		= 0x2000,
	USB_CTRL		= 0x2010,
	USB_STAT		= 0x2014,
	USB_EPA_CFG		= 0x2144,
	USB_EPA_CTL		= 0x2148,
	USB_EPA_MAXPKT		= 0x2158,
	USB_EPA_MAXPKT_2	= 0x215a,
	USB_EPA_FIFO_CFG	= 0x2160,
};

enum blocks {
	DEMODB			= 0,
	USBB			= 1,
	SYSB			= 2,
	TUNB			= 3,
	ROMB			= 4,
	IRB			= 5,
	IICB			= 6,
};

struct rtlsdr_dev {
	libusb_context *ctx;
	struct libusb_device_handle *devh;
	uint32_t xfer_buf_num;
	uint32_t xfer_buf_len;
	struct libusb_transfer **xfer;
	unsigned char **xfer_buf;
	rtlsdr_read_async_cb_t cb;
	void *cb_ctx;
	enum rtlsdr_async_status async_status;
	int async_cancel;
	int use_zerocopy;
	/* rtl demod context */
	uint32_t rate; /* Hz */
	uint32_t rtl_xtal; /* Hz */
	int fir[FIR_LEN];
	int direct_sampling;
	/* tuner context */
	enum rtlsdr_tuner tuner_type;
	rtlsdr_tuner_iface_t *tuner;
	uint32_t tun_xtal; /* Hz */
	uint32_t freq; /* Hz */
	uint32_t bw;
	uint32_t offs_freq; /* Hz */
	int corr; /* ppm */
	int gain; /* tenth dB */
	struct e4k_state e4k_s;
	struct r82xx_config r82xx_c;
	struct r82xx_priv r82xx_p;
	/* status */
	int dev_lost;
	int driver_active;
	unsigned int xfer_errors;
	char manufact[256];
	char product[256];
	int force_bt;
	enum rtlsdr_ds_mode direct_sampling_mode;
};

/*
 * Tables defined in librtlsdr.c.
 *
 * `tuners` is indexed by `enum rtlsdr_tuner`; the bridge assigns `dev->tuner` from it after
 * probing. `fir_default` is the FIR coefficient set the bridge copies into a freshly
 * allocated device. Both were `static` while librtlsdr.c was included inline; they are now
 * linked normally and kept inside the library by `-fvisibility=hidden` on the target.
 */
extern const int fir_default[FIR_LEN];
extern rtlsdr_tuner_iface_t tuners[];

/*
 * Register-level helpers the Android bridge uses while assembling a device.
 *
 * These are not `static` in librtlsdr.c and never were -- they simply had no declaration
 * outside it. `rtlsdr_set_if_freq()` is the one exception: it lost its `static` in Phase 1
 * so the bridge can set the R82xx IF frequency after probing.
 */
uint8_t rtlsdr_i2c_read_reg(rtlsdr_dev_t *dev, uint8_t i2c_addr, uint8_t reg);
int rtlsdr_write_reg(rtlsdr_dev_t *dev, uint8_t block, uint16_t addr, uint16_t val, uint8_t len);
int rtlsdr_demod_write_reg(rtlsdr_dev_t *dev, uint8_t page, uint16_t addr, uint16_t val, uint8_t len);
void rtlsdr_set_i2c_repeater(rtlsdr_dev_t *dev, int on);
void rtlsdr_set_gpio_output(rtlsdr_dev_t *dev, uint8_t gpio);
void rtlsdr_set_gpio_bit(rtlsdr_dev_t *dev, uint8_t gpio, int val);
void rtlsdr_init_baseband(rtlsdr_dev_t *dev);
int rtlsdr_set_if_freq(rtlsdr_dev_t *dev, uint32_t freq);

/*
 * Reads one USB string descriptor out of an EEPROM image and returns the offset of the next.
 * The caller's `str` must hold at least 256 bytes.
 */
int get_string_descriptor(int pos, uint8_t *data, char *str);

#ifdef __cplusplus
}
#endif

#endif /* __LIBRTLSDR_INTERNAL_H */
