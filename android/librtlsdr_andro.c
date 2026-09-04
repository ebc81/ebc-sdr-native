/*
 * librtlsdr_wrapper is na addition to the original librtlsdr
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
 *
 * Modification 2016 by Christian Ebner <cebner@gmx.at>
 * rtlsdr_open2 base on librtlsdr_andro from Martin Marinov <martintzvetomirov@gmail.com> 2012
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

/* __EBCANDROID__: these came in transitively while this file #included librtlsdr.c.
 * Listed explicitly since Phase 1. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "librtlsdr_andro.h"

#include "ebc_log.h"

/* __EBCANDROID__: this used to be #include "rtl_ais_andro.h" and "rtlaisjava_err.h" --
 * app-owned headers, which is why this file could not be compiled outside
 * RTL_SDR_AIS_Driver. ebc_log.h now provides aprintf_stderr() and librtlsdr_andro.h the
 * error codes. Phase 1, Vorarbeit 4.3. */

/* __EBCANDROID__: aprintf_info() for what is not an error.
 *
 * aprintf_stderr() maps to ANDROID_LOG_ERROR, inherited from rtlsdrPager's macro. Every
 * normal open therefore used to emit "Found Rafael Micro R828D tuner" and "RTL-SDR Blog V4
 * Detected" at ERROR level -- verified in a logcat capture on 2026-09-04. Tuner discovery,
 * a kernel-driver detach and the reset fallback are progress notes, not failures; a log that
 * cries wolf on every start is a log nobody reads when something is actually wrong. */
#include "librtlsdr_internal.h"

/* __EBCANDROID__: this used to be #include "rtl-sdr/src/librtlsdr.c" -- the whole
 * implementation pulled in inline, because this file needs the private
 * struct rtlsdr_dev. librtlsdr_internal.h now exposes exactly that much, so librtlsdr.c
 * is a normal translation unit and this file no longer pins the directory layout of the
 * two trees. Phase 1, Vorarbeit 4.1. */
//
//https://github.com/kuldeepdhaka/libusb/tree/android-open2
//
//How To for Android
//==================

//1. Search for the UsbDevice [1] you are interested in.
//2. Extract the usbfs path [2] from the UsbDevice.
//3. Build a libbox0_device from the path
//using libusb_get_device2(context, path) [3]
//4. open the UsbDevice [4], you will get UsbDeviceConnection [5].
//5. from the UsbDeviceConnection, extract the fd[6]
//6. now, from the fd and the previously
//created libusb_device, build a libusb_device_handle
//by calling libusb_get_device2(dev, handle, fd)[7].
//7. and that all you need.
//[1] http://developer.android.com/reference/android/hardware/usb/UsbDevice.html
//[2] http://developer.android.com/reference/android/hardware/usb/UsbDevice.html#getDeviceName%28%29
//[3] https://github.com/kuldeepdhaka/libusb/blob/android-open2/libusb/core.c#L1492
//[4] http://developer.android.com/reference/android/hardware/usb/UsbManager.html#openDevice%28android.hardware.usb.UsbDevice%29
//[5] http://developer.android.com/reference/android/hardware/usb/UsbDeviceConnection.html
//[6] http://developer.android.com/reference/android/hardware/usb/UsbDeviceConnection.html#getFileDescriptor%28%29
//[7] https://github.com/kuldeepdhaka/libusb/blob/android-open2/libusb/core.c#L1267


/*
 * The libusb error behind the last rtlsdr_open2() failure.
 *
 * rtlsdr_open2() collapses every claim failure into EBC_SDR_ERR_CLAIM, so the caller
 * cannot tell "another process still holds the interface" from a genuinely broken dongle.
 * That distinction is worth a different sentence to the user -- an immediate re-plug fails
 * with LIBUSB_ERROR_BUSY because the kernel has not finished releasing the interface from
 * the previous session, and the honest advice is to wait a moment, not "cannot open".
 *
 * Single-session by design (one dongle, one capture loop), so a single value is enough.
 */
static int g_last_open_libusb_err = 0;

/*
 * __EBCANDROID__: Phase 1, Vorarbeit 4.2 -- the signature was
 *
 *     rtlsdr_open2(rtlsdr_dev_t **out_dev, uint32_t index, int fd, const char *uspfs_path)
 *
 * in RTL_SDR_AIS_Driver, inherited from Martin Marinov's 2012 wrapper, where `index` and
 * `uspfs_path` were needed to find the device. They have been dead parameters ever since
 * libusb_wrap_sys_device() replaced that path: Android hands over a file descriptor and
 * there is nothing left to enumerate or look up. rtlsdr433 and rtlsdrPager had already
 * dropped them. Two parameters is now the one form.
 */
int rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd)
{
	int r;
	//int i;
	//libusb_device **list;
	rtlsdr_dev_t *dev = NULL;
	//libusb_device *device = NULL;
	//uint32_t device_count = 0;
	//struct libusb_device_descriptor dd;
	uint8_t reg;
	//ssize_t cnt;
    uint8_t buf[EEPROM_SIZE];
    int pos;
    int eeprom_ok = 0;

	g_last_open_libusb_err = 0;

	dev = malloc(sizeof(rtlsdr_dev_t));
	if (NULL == dev)
		return -ENOMEM;

	memset(dev, 0, sizeof(rtlsdr_dev_t));
	memcpy(dev->fir, fir_default, sizeof(fir_default));


	int status = libusb_init(&dev->ctx);
	if (status != LIBUSB_SUCCESS)
	{
		aprintf_stderr("libusb_init failed");
		free(dev);
		return status;
	}
	else if (dev->ctx == NULL)
	{
		free(dev);
		dev = NULL;
		//return LIBUSB_ERROR_OTHER;
		return EBC_SDR_ERR_LIBUSB_INIT;
	}

	dev->dev_lost = 1;

    //libusb_set_option(dev->ctx,LIBUSB_OPTION_LOG_LEVEL,LIBUSB_LOG_LEVEL_WARNING);

	status = libusb_wrap_sys_device(dev->ctx,fd,&dev->devh);
	if ( status != LIBUSB_SUCCESS)
    {
        aprintf_stderr("libusb_wrap_sys_device failed (%d)", status);
        libusb_exit(dev->ctx);
        free(dev);
        return status;
    }

	if (libusb_kernel_driver_active(dev->devh, 0) == 1) {
		dev->driver_active = 1;

#ifdef DETACH_KERNEL_DRIVER
		if (!libusb_detach_kernel_driver(dev->devh, 0)) {
			aprintf_info("Detached kernel driver\n");
		} else {
			aprintf_stderr("Detaching kernel driver failed!");
			goto err;
		}
#else
		aprintf_stderr("\nKernel driver is active, or device is "
							   "claimed by second instance of librtlsdr."
							   "\nIn the first case, please either detach"
							   " or blacklist the kernel module\n"
							   "(dvb_usb_rtl28xxu), or enable automatic"
							   " detaching at compile time.\n\n");
#endif
	}

	r = libusb_claim_interface(dev->devh, 0);
	if (r < 0) {
		aprintf_stderr("usb_claim_interface error %d\n", r);
		g_last_open_libusb_err = r;
		r = EBC_SDR_ERR_CLAIM;
		goto err;
	}

	dev->rtl_xtal = DEF_RTL_XTAL_FREQ;

	/* perform a dummy write, if it fails, reset the device */
	if (rtlsdr_write_reg(dev, USBB, USB_SYSCTL, 0x09, 1) < 0) {
		aprintf_info("Resetting device...\n");
		libusb_reset_device(dev->devh);
	}

	rtlsdr_init_baseband(dev);
	dev->dev_lost = 0;

    /* Get device manufacturer and product id
	 * NOTE: The standard way to get these strings from libusb doesn't work on Android
	 * Instead grab them directly from the EEPROM
	 * */

    /* buf must be zeroed and the result checked: rtlsdr_read_eeprom() can fail
     * (-1/-2/-3) after filling none or only part of the buffer, leaving the rest
     * indeterminate. Parsing that garbage walked off the end of this 256-byte stack
     * buffer via the unbounded length byte in get_string_descriptor(). */
    memset(buf, 0, sizeof(buf));
    r = rtlsdr_read_eeprom(dev, buf, 0, EEPROM_SIZE);
    eeprom_ok = (r >= 0);
    if (eeprom_ok) {
        pos = get_string_descriptor(STR_OFFSET, buf, dev->manufact);
        get_string_descriptor(pos, buf, dev->product);
    } else {
        aprintf_stderr("EEPROM read failed (%d); USB strings and the Bias-T EEPROM hack are skipped\n", r);
        dev->manufact[0] = '\0';
        dev->product[0]  = '\0';
    }

	/* Probe tuners */
	rtlsdr_set_i2c_repeater(dev, 1);

	reg = rtlsdr_i2c_read_reg(dev, E4K_I2C_ADDR, E4K_CHECK_ADDR);
	if (reg == E4K_CHECK_VAL) {
		aprintf_info("Found Elonics E4000 tuner\n");
		dev->tuner_type = RTLSDR_TUNER_E4000;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg(dev, FC0013_I2C_ADDR, FC0013_CHECK_ADDR);
	if (reg == FC0013_CHECK_VAL) {
		aprintf_info("Found Fitipower FC0013 tuner\n");
		dev->tuner_type = RTLSDR_TUNER_FC0013;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg(dev, R820T_I2C_ADDR, R82XX_CHECK_ADDR);
	if (reg == R82XX_CHECK_VAL) {
		aprintf_info("Found Rafael Micro R820T tuner\n");
		dev->tuner_type = RTLSDR_TUNER_R820T;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg(dev, R828D_I2C_ADDR, R82XX_CHECK_ADDR);
	if (reg == R82XX_CHECK_VAL) {
		aprintf_info("Found Rafael Micro R828D tuner\n");
		if (rtlsdr_check_dongle_model(dev, "RTLSDRBlog", "Blog V4"))
			aprintf_info("RTL-SDR Blog V4 Detected\n");
		if (rtlsdr_check_dongle_model(dev, "RTLSDRBlog", "Blog V4L"))
			aprintf_info("RTL-SDR Blog V4L Detected\n");


		dev->tuner_type = RTLSDR_TUNER_R828D;
		goto found;
	}

    /* initialise GPIOs */
    rtlsdr_set_gpio_output(dev, 4);

    /* reset tuner before probing */
    rtlsdr_set_gpio_bit(dev, 4, 1);
    rtlsdr_set_gpio_bit(dev, 4, 0);

	reg = rtlsdr_i2c_read_reg(dev, FC2580_I2C_ADDR, FC2580_CHECK_ADDR);
	if ((reg & 0x7f) == FC2580_CHECK_VAL) {
		aprintf_info("Found FCI 2580 tuner\n");
		dev->tuner_type = RTLSDR_TUNER_FC2580;
		goto found;
	}

	reg = rtlsdr_i2c_read_reg(dev, FC0012_I2C_ADDR, FC0012_CHECK_ADDR);
	if (reg == FC0012_CHECK_VAL) {
		aprintf_info("Found Fitipower FC0012 tuner\n");
		rtlsdr_set_gpio_output(dev, 6);
		dev->tuner_type = RTLSDR_TUNER_FC0012;
		goto found;
	}

	found:
	/* use the rtl clock value by default */
	dev->tun_xtal = dev->rtl_xtal;
	dev->tuner = &tuners[dev->tuner_type];

	switch (dev->tuner_type) {
		case RTLSDR_TUNER_R828D:
			/* If NOT an RTL-SDR Blog V4, set typical R828D 16 MHz freq. Otherwise, keep at 28.8 MHz. */
			if (!(rtlsdr_check_dongle_model(dev, "RTLSDRBlog", "Blog V4"))) {
				dev->tun_xtal = R828D_XTAL_FREQ;
			}
			/* fall-through */
		case RTLSDR_TUNER_R820T:
            /* disable Zero-IF mode */
            rtlsdr_demod_write_reg(dev, 1, 0xb1, 0x1a, 1);

            /* only enable In-phase ADC input */
            rtlsdr_demod_write_reg(dev, 0, 0x08, 0x4d, 1);

            /* the R82XX use 3.57 MHz IF for the DVB-T 6 MHz mode, and
             * 4.57 MHz for the 8 MHz mode */
            rtlsdr_set_if_freq(dev, R82XX_IF_FREQ);

            /* enable spectrum inversion */
            rtlsdr_demod_write_reg(dev, 1, 0x15, 0x01, 1);
			break;
		case RTLSDR_TUNER_UNKNOWN:
			aprintf_stderr("No supported tuner found\n");
			rtlsdr_set_direct_sampling(dev, 1);
			break;
		default:
			break;
	}
	/* Hack to force the Bias T to always be on if we set the IR-Endpoint
	* bit in the EEPROM to 0. Default on EEPROM is 1.
	*
	* Only honoured when the EEPROM actually read back. Otherwise buf is all
	* zeroes, (buf[7] & 0x02) is 0, and force_bt would become 1 -- switching the
	* Bias-T on unrequested and putting 5 V on the antenna connector, which both
	* risks passive-antenna hardware and contradicts the Bias-T OFF default.
	*/
	dev->force_bt = (eeprom_ok && !(buf[7] & 0x02)) ? 1 : 0;
	if(dev->force_bt)
		rtlsdr_set_bias_tee(dev, 1);


	if (dev->tuner->init) {
		r = dev->tuner->init(dev);
		if (r < 0)
			goto err;
	}

	rtlsdr_set_i2c_repeater(dev, 0);

	*out_dev = dev;

	return 0;
	err:
	if (dev) {
        if (dev->devh)
            libusb_close(dev->devh);

        if (dev->ctx)
			libusb_exit(dev->ctx);

		if ( dev )
            free(dev);
	}

	return r;
}


int rtlsdr_cancel_async_save_fast(rtlsdr_dev_t *dev)
{
	if ( !dev) {
		return -1;
	}
	if ( dev->async_status == RTLSDR_RUNNING && dev->dev_lost == 0)
	{
		return rtlsdr_cancel_async(dev);
	}
	return 0;


}

int rtlsdr_cancel_async_save(rtlsdr_dev_t *dev)
{
#define MY_TIMEOUT      1000//msec
	int r;


	if ( !dev) {
		//__android_log_write(ANDROID_LOG_DEBUG,"NATIVE","rtlsdr_cancel_async_save  dev = null");
		return -1;
	}

	/*
	if ( dev->async_status == RTLSDR_CANCELING)
	{
		__android_log_write(ANDROID_LOG_DEBUG,"NATIVE","rtlsdr_cancel_async_save  CANCELLING");
	}
	if ( dev->async_status == RTLSDR_RUNNING)
		__android_log_write(ANDROID_LOG_DEBUG,"NATIVE","rtlsdr_cancel_async_save  RUNNING");
*/
	if ( dev->async_status == RTLSDR_RUNNING && dev->dev_lost == 0)
	{
		//__android_log_write(ANDROID_LOG_DEBUG,"NATIVE","rtlsdr_cancel_async");
		r = rtlsdr_cancel_async(dev);
		// The driver needs some time until cancel_async flag has closed all things
		int timeout = 0;
		while (timeout < MY_TIMEOUT) {
			usleep(1000); //1 ms
			timeout++;
			if ( dev->async_status ==  RTLSDR_INACTIVE || dev->dev_lost)
				break;
		}
		return r;
	}
	//__android_log_write(ANDROID_LOG_DEBUG,"NATIVE","rtlsdr_cancel_async_save  END");
	return 0;
}

/**
 * Non-zero if the last rtlsdr_open2() failed because the interface was already claimed.
 *
 * Kept as a separate query rather than widening rtlsdr_open2()'s return, so the app layer
 * needs no libusb header of its own.
 */
int rtlsdr_last_open_was_busy(void)
{
	return g_last_open_libusb_err == LIBUSB_ERROR_BUSY;
}

/**
 * Non-zero once libusb has reported the device gone -- i.e. the dongle was unplugged.
 *
 * dev_lost lives in the private struct rtlsdr_dev, which only this file can see because it
 * #includes librtlsdr.c inline. Without this accessor an unplug is indistinguishable from a
 * read failure, and the user is told the wrong thing.
 */
int rtlsdr_is_dev_lost(rtlsdr_dev_t *dev)
{
	return dev ? dev->dev_lost : 0;
}
