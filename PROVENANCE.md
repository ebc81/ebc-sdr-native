# PROVENANCE — where this code comes from and every way it differs

**Tree:** shared native SDR base for the EBC Android apps (working title `ebc-sdr-native`)
**Created:** 2026-09-04 (Phases 0 and 1 of [KONZEPT-GEMEINSAME-CODEBASE.md](KONZEPT-GEMEINSAME-CODEBASE.md))
**Status:** builds as a static library, `ebc_sdr`. **All three apps use it**, each pinning tag
`v0.3.0` as a submodule, and each verified on a Blog V4 (§5). Phases 0 to 4 are done; Phase 5
(the GPL source paths on the app side) is open.

This file is the contract: **every byte that differs from upstream is listed here with a
reason.** If you change a vendored file, add the entry in the same commit. `git log` and this
file must agree; the check is in [Verification](#verification).

---

## 1. Upstream baselines

| Component | Upstream | Version | Commit | Vendored via |
| --- | --- | --- | --- | --- |
| `rtl-sdr/` | [osmocom/rtl-sdr](https://github.com/osmocom/rtl-sdr) | v2.0.3 | `797f814` | `RTL_SDR_AIS_Driver`, which had aligned it against the Blog fork in May 2026 |
| `rtl-sdr/` (features) | [rtlsdrblog/rtl-sdr-blog](https://github.com/rtlsdrblog/rtl-sdr-blog) | V1.4.0 | `aed0ea1` | three features only, see §3.2 |
| `libusb-andro/` | [libusb/libusb](https://github.com/libusb/libusb) | 1.0.23 (`LIBUSB_NANO 11397`) | — | `RTL_SDR_AIS_Driver`, own maintenance line |
| `android/` | no upstream | — | — | EBC + Martin Marinov's `rtlsdr_open2` idea (2012) |

Line endings are **LF** everywhere, matching what all three upstreams store. `.gitattributes`
pins this with `* -text`; see that file for why.

### Why the AIS tree is the base

Of the three app copies, `RTL_SDR_AIS_Driver` was the most advanced (ANALYSE.md §8): it is
osmocom v2.0.3 plus the worthwhile Blog features, has 8 commits touching the libs against 3
(rtlsdr433) and 0 (rtlsdrPager), and it is the only copy verified on hardware and against real
Play Console crashes. `rtlsdr433` and `rtlsdrPager` are byte-identical to each other in
`rtl-sdr/`, so there were only ever **two** variants to merge, not three.

Commit `6643319` imported it byte-for-byte; `176ad55` normalised the line endings. After
those two commits 47 of the 49 files were byte-identical to what the AIS repo stores.

### What was deliberately left out of the import

| Left out | Why |
| --- | --- |
| `rtl-sdr/src/getopt/` | Not compiled by any of the three apps. |
| `librtlsdr.c.bak`, `tuner_r82xx.c.bak` (rtlsdr433 only) | Unmodified Blog code from 2023-10-30. Useful as a merge-base reference, does not belong in a source tree. |
| `aisdecoder/`, `rtl_ais_andro.*`, `rtlaisjava*` | App-specific, not part of the shared base. |

`rtl-sdr/src/convenience/` **is** included, but it is an optional module behind
`EBC_SDR_CONVENIENCE`: only `RTL_SDR_AIS_Driver` compiles it.

---

## 2. Files that are byte-identical to upstream

Verified against `tmp/osmocom-rtl-sdr` at `797f814`, line-ending neutral:

- `rtl-sdr/src/tuner_fc0012.c`
- `rtl-sdr/src/tuner_fc0013.c`
- `rtl-sdr/src/tuner_fc2580.c`
- `rtl-sdr/include/tuner_r82xx.h`

Everything else in `rtl-sdr/` carries at least one documented patch. `libusb-andro/` differs
from libusb 1.0.23 across the Android layer and the unplug fixes (§3.4).

Phase 1 added two files with no upstream at all: `rtl-sdr/src/librtlsdr_internal.h` and
`android/ebc_log.h` (§3.8). `libusb-andro/libusb/config.h` was removed (§3.8, P7).

---

## 3. Every patch, with its reason

The **Archive commit** column in the tables below points into `ebc81/ebc-sdr-native-internal`,
a private repository holding the granular construction history of this tree — the vendored
import, each backport, each finding fix, the library work. This repository starts with the
finished tree, so those hashes do not resolve here. That is deliberate: the useful record is
the prose below, and it is complete without the hashes. They are kept so the author can find
the exact change.

The **Commit** column in §1 is different — those are upstream commits in osmocom/rtl-sdr and
rtlsdrblog/rtl-sdr-blog.

### 3.1 Backported from rtlsdr433 / rtlsdrPager in Phase 0

| # | Patch | From | Archive commit | Reason |
| --- | --- | --- | --- | --- |
| B1 | Five narrowing casts — `librtlsdr.c` `rtlsdr_set_if_freq` `(int32_t)`, `rtlsdr_set_sample_freq_correction` `(int16_t)`, `rtlsdr_set_sample_rate` `(uint32_t)`; `tuner_e4k.c` `compute_flo` and `e4k_compute_pll_params` `(uint32_t)` | rtlsdr433 | `c2d00b8` | All five compute in `double` via `TWO_POW()` and fall back to integers. **Not cosmetic:** without them the AIS tree fails to compile under `-Werror=shorten-64-to-32`, which AIS applies to the translation unit that inline-includes `librtlsdr.c`. Verified — see §5. |
| B2 | `if (!dev) return -1;` in `rtlsdr_set_direct_sampling()` | rtlsdr433 | `73e5126` | The public wrapper dereferenced `dev->direct_sampling_mode` before any NULL check. The inner `_rtlsdr_set_direct_sampling()` already had the guard. Befund 8. |
| B3 | `rtlsdr_last_open_was_busy()` and `rtlsdr_is_dev_lost()` plus the carrier state `g_last_open_libusb_err` | rtlsdrPager | `9023a05` | `rtlsdr_open2()` folds every claim failure into `EXIT_TEST_RTLSDR_OPEN2`, so the caller cannot tell "interface still held" (`LIBUSB_ERROR_BUSY` right after a re-plug — waiting helps) from a broken dongle. `dev_lost` lives in the private `struct rtlsdr_dev`, so without an accessor an unplug is indistinguishable from a read error. Separate queries rather than a widened return value, so the app layer needs no libusb header. |
| B4 | `linux_usbfs.c` — the clearer comment on the `handle_bulk_completion` reap guard | rtlsdrPager | `7118c3f` | The code is identical in AIS and Pager; only the comment differed. Pager's names the TOCTOU window explicitly and carries the `__EBCANDROID__` marker. Merged with AIS's v1.3.20 note; Pager's provenance sentences were replaced by the actual fix history. **No code merged from rtlsdr433** — that copy is missing this guard and the `usercontext` zeroing (Befund 3, severity high). |

### 3.2 Blog features kept (already present in the AIS tree)

- `enum rtlsdr_ds_mode`, `direct_sampling_mode`, `_rtlsdr_set_direct_sampling()` and the
  auto switch to direct sampling below 24 MHz on R820T (V4L excluded — it has its own HF
  upconverter).
- `force_bt` from EEPROM byte 7 and the interlock in `rtlsdr_set_bias_tee_gpio()`.
- The public declaration of `rtlsdr_check_dongle_model()`, which osmocom does not export.

### 3.3 Blog hacks deliberately discarded

Per ANALYSE.md §6.2/§6.3. **rtlsdr433 and rtlsdrPager changed hardware behaviour when they
moved to this tree** — that is what the Phase-2 and Phase-3 hardware verification was for.
Both are through it; see §5.

| Hack | Why discarded |
| --- | --- |
| VGA gain re-set on every tune (`r82xx_set_vga_gain()` in `r82xx_set_freq`) | Pins register `0x0c` to 16.3 dB while `r82xx_set_gain()` sets 26.5 dB in AGC mode — **the AGC loses ~10 dB on every frequency change.** Befund 5. Directly visible: change frequency repeatedly with AGC on and watch the noise floor. The declaration `r82xx_set_vga_gain()` is gone from `tuner_r82xx.h` too. |
| L-band dropout tweak (`div_buf_cur = 0xa0`) | Only acts above ~1 GHz. Irrelevant for AIS (162 MHz), 433 MHz ISM and POCSAG. |
| Bias tee via `rtlsdr_set_offset_tuning()` | See P2 below. |
| VCO current at maximum (`0x12`, `0x06/0xff` instead of `0x80/0xe0`) | The `0xff` mask also overwrites the `pw_sdm` bits that osmocom sets on purpose, so the two approaches cannot be combined. **Decided on hardware: the osmocom value stays** — see §6. |
| `r82xx_toggle_test()` | Debug function. |
| `uint32_t rf_freq` in `struct r82xx_priv` | Unused; only reference was a commented-out line. Removed in `61a1490`. |

Also not taken: the Blog/433 form of `get_string_descriptor()` (reads `data[pos]` unbounded —
Befund 2), the `convenience.c` `set_gain_by_perc()` clamp that indexes a zero-byte allocation
(Befund 4), `rtlsdr_cancel_async()` called from inside the transfer callback (Befund 7), and
the V4L detection message in the R828D instead of the R820T branch (diagnostics only, but it
never fires on real V4L hardware).

### 3.4 Patches inherited from the AIS tree

Ported from osmocom mainline or EBC's own hardening. Listed so a future rebase knows what to
carry (ANALYSE.md §5.3).

**`rtl-sdr/src/tuner_r82xx.c`** — `shadow_equal()` (skips an I2C write when the value is
already in the register), `mask_reg8()` and the bundled block write of registers 0x10–0x16
(one transaction per tune instead of six — on Android every I2C transaction is a control
transfer with a ~300 ms timeout), the exact fixed-point PLL (`vco_div`) instead of the
iterative SDM loop, `memset(priv->regs, 0, NUM_REGS)` at init, and the `val -= r` fix in
`shadow_store()`.

**`rtl-sdr/src/librtlsdr.c`** — `dev_lost` flag plus the check in the async loop (instead of
cancelling from the callback), the resubmit return-value check in the transfer callback,
NULL guards throughout `rtlsdr_close()`, the bounds check in `get_string_descriptor()`,
`rtlsdr_set_i2c_repeater(dev, 0)` in the direct-sampling branch, kernel-driver detach, GPIO
reset before the FC2580/FC0012 probe, and the libusb compatibility macro.

**`rtl-sdr/src/convenience/convenience.c`** — `set_gain_by_perc()` checks `count <= 0`, clamps
`percent` to 100 and handles `malloc` failure. The upstream clamp produces `0xFFFFFFFF` on
tuners without a gain table and indexes a zero-byte allocation.

**`libusb-andro/`** — enumeration and netlink hotplug disabled under `#if defined(__ANDROID__)`,
`fd_keep` in `libusb_wrap_sys_device`, `__android_log` as the log sink, and the Android
14/15/16 unplug fixes: the stale-URB guard in `handle_bulk_completion` (checked **inside**
`itransfer->lock`), `usercontext` zeroed before `free(tpriv->urbs)` in two places, and a NULL
guard in `reap_for_handle`.

**`android/librtlsdr_andro.c`** — the whole file. `rtlsdr_open2()` builds the private
`rtlsdr_dev_t` itself around `libusb_wrap_sys_device(fd)` instead of going through
`rtlsdr_open()`, because Android has no device enumeration. Includes the EEPROM hardening
(`memset` of the buffer, `eeprom_ok`, tuner-init failure leads to `goto err`) that §3.5/P1
mirrors into `librtlsdr.c`.

The resubmit check and the `rtlsdr_close()` NULL guards exist **in all three apps and in
neither upstream** — they are candidates for an upstream patch (§7).

### 3.5 Findings fixed in Phase 0

| # | Finding | Severity | Archive commit | What changed |
| --- | --- | --- | --- | --- |
| P1 | Befund 1 — `rtlsdr_open()` ignored the return value of `rtlsdr_read_eeprom()` | **high** | `1cb0e53` | On a read error `buf[7]` came from uninitialised stack, so `force_bt` could become 1 and **put 5 V on the antenna connector** of a device whose EEPROM never asked for it. Now `memset(buf, ...)` plus an `r < 0` branch, the same hardening `librtlsdr_andro.c` already had. The bug is also in the Blog upstream. |
| P2 | Befund 9 — bias tee switched by `rtlsdr_set_offset_tuning()` | low | `1cb0e53` | The Blog hack turned the bias tee on for R820T/R828D and then returned `-2` ("not supported"). Removed: a call that answers "not supported" must not put 5 V on the antenna as a side effect. Now osmocom behaviour, as in rtlsdr433/rtlsdrPager. **Behaviour change for `RTL_SDR_AIS_Driver`** — see [CHANGELOG.md](CHANGELOG.md). |
| P3 | Befund 6 — FC0013 register 0x14 `0x20` instead of `0x40` | medium | — | **No code change needed:** the AIS tree already has `0x40` at both sites. rtlsdr433/rtlsdrPager have `0x20` at the second one, which is the wrong band switch ("enable UHF & disable GPS") and came in with their initial import. Those two apps get a behaviour **correction** on FC0013 dongles when they move to this tree. |
| P4 | Befund 10 — library diagnostics lost on Android | low | `b5f724e` | The unused `<android/log.h>` include is gone. **The other half is deliberately still open:** all 83 `fprintf(stderr, ...)` calls are spread over `librtlsdr.c` (36), `convenience.c` (32), `tuner_r82xx.c` (7), `tuner_e4k.c` (6), `tuner_fc0012.c` and `tuner_fc0013.c` (1 each), so a redirect cannot live in one file — it needs the shared `android/ebc_log.h` from Vorarbeit 4.3, and overriding `fprintf()` wants a real build across all four ABIs and against API 23 before it is handed to three apps. Phase 0 has no CMake target. The site is marked `__EBCANDROID__` so Phase 1 finds it. |

### 3.6 Comment-only changes

`37a5342`, `00600e6` — where the two variants disagreed only in wording, the clearer one won:
the doxygen block for `rtlsdr_check_dongle_model()` and the auto-direct-sampling comment came
from rtlsdrPager; the `enum rtlsdr_ds_mode` comments stayed with the AIS tree, which names the
physical branch and where HF sits on a v3 dongle rather than restating the enum names.

`61a1490` — three spots restored to the upstream wording or whitespace, listed in §3.3 and the
commit message. In `tuner_fc0013.c` the local comment contradicted the code it described.

### 3.7 The `__EBCANDROID__` marker

In this tree `__EBCANDROID__` appears **in comments only** — it marks a deliberate deviation
from the vendored upstream so `grep -rn __EBCANDROID__` finds them all. It is not a compile
guard here (rtlsdrPager does use it as one, via `-D__EBCANDROID__=1`, for its `multimon/`
tree).

**Phase 1 decided to keep it that way.** Nothing in this tree needs a compile-time switch:
the library is Android-only, so there is no non-Android branch to select. Defining
`__EBCANDROID__` on the target would add a knob nobody reads. The one place that does
branch on the platform is `ebc_log.h`, and it uses the NDK's own `__ANDROID__` so the tree
still preprocesses on a host for analysis.

`grep -rn __EBCANDROID__ rtl-sdr libusb-andro android` currently finds 17 markers across
11 files. Every local deviation should have one.

---

### 3.8 Phase 1 — turning the tree into a library

The Vorarbeiten of KONZEPT-GEMEINSAME-CODEBASE.md §4. Each one is a change to the tree,
so each one is listed here.

| # | Vorarbeit | Archive commit | What changed |
| --- | --- | --- | --- |
| P5 | 4.1 — decouple `librtlsdr.c` | `d2a902b` | **The Phase 1 blocker.** `android/librtlsdr_andro.c` pulled the whole implementation in with `#include "rtl-sdr/src/librtlsdr.c"`, because it needs the private `struct rtlsdr_dev`: on Android there is no enumeration, so `rtlsdr_open2()` assembles the device itself around `libusb_wrap_sys_device(fd)` instead of calling `rtlsdr_open()`. Cost: `librtlsdr.c` could never be a translation unit (listing it in CMake next to the bridge gave duplicate symbols), and the relative layout of the two trees was frozen. New internal header `rtl-sdr/src/librtlsdr_internal.h` carries what the bridge needs — `struct rtlsdr_dev`, `rtlsdr_tuner_iface_t`, `enum rtlsdr_async_status`, `FIR_LEN`/`EEPROM_SIZE`/`STR_OFFSET`, `DEF_RTL_XTAL_FREQ`, the `usb_reg` and `blocks` register enums, and declarations of the register-level helpers. Contents **moved verbatim**, not rewritten. |
| P6 | 4.3 — logging and error codes | `d2a902b`, `b70fa64` | New `android/ebc_log.h`. `aprintf_stderr` was a *function* from AIS's `rtl_ais_andro.h` and a *macro* in the other two; it is a macro here with the tag configurable via `EBC_LOG_TAG`. Own error range `EBC_SDR_ERR_*` from -2000 in `librtlsdr_andro.h`, with the mapping table onto each app's codes. Together these are what let the bridge compile without an app. Also closes Befund 10 — see below. |
| P7 | 4.4 — one `config.h` | `d2a902b` | `libusb-andro/libusb/config.h` deleted. The two differed in exactly one line — `/* #undef ENABLE_DEBUG_LOGGING */` against `#undef ENABLE_DEBUG_LOGGING` — and both leave the macro undefined, so they were semantically identical. `libusb-andro/config.h` is kept: that is where libusb expects its generated `config.h` upstream, and the one AIS already resolved to. `libusb-andro/` is PRIVATE on the target, so `<config.h>` from `libusbi.h:26` is unambiguous. |
| P8 | 4.2 — one `rtlsdr_open2()` | `ead52d0` | Was `rtlsdr_open2(rtlsdr_dev_t **, uint32_t index, int fd, const char *uspfs_path)` in AIS, inherited from Martin Marinov's 2012 wrapper where `index` and `uspfs_path` located the device. Dead parameters since `libusb_wrap_sys_device()`; rtlsdr433 and rtlsdrPager had already dropped them. Now `rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd)` everywhere. |
| P9 | 4.8 — Befund 11 | `ead52d0` | `rtlsdr_supporting_ppm_search()` was declared in all three `librtlsdr_andro.h` and defined only in AIS's, so two apps carried a dangling declaration. Consolidating the bridge fixed that by giving it one definition — then P11 removed it outright. |
| P10 | 4.5, 4.6, 4.7 — the target | `e7f1e0e` | `CMakeLists.txt`: `add_library(ebc_sdr STATIC ...)`. Compile options move from AIS's global `CMAKE_C_FLAGS` onto the target; the 16 KB page-alignment linker flags become `target_link_options(INTERFACE)` so every consumer inherits them; API 23 stays the floor. `convenience.c` becomes an opt-in module. See §4. |
| P11 | `rtlsdr_supporting_ppm_search()` removed | see CHANGELOG | Dropped from the public surface. It had no caller anywhere — not in native code, and not across JNI: AIS's `support_ppm_search()` JNI method returns a hardcoded `JNI_FALSE` and never called it. It only ever returned 1. P9 had kept it on the grounds that dropping a published symbol while the apps still linked their own copies bought nothing; with rtlsdrPager migrated that no longer holds, and the whole PPM-search subsystem it belonged to is being removed from RTL_SDR_AIS_Driver as well. |

**Befund 10 is now fully closed** (`d2a902b`, `b70fa64`). All 83 `fprintf(stderr, ...)`
diagnostics of the library — tuner detection, PLL failures, the exact sample rate, "No
supported tuner found" — went nowhere on Android, because there is no stderr sink. Phase 0
could only remove the unused `<android/log.h>` include; the redirect needed a shared header
and a real build to verify, both of which Phase 1 has.

`ebc_log.h` redirects `fprintf` rather than touching the 83 call sites, so the vendored
sources stay diffable against upstream. The redirect is **not blind**: the shim inspects the
stream, so a genuine `fprintf` to a file still goes to that file; only stderr (WARN) and
stdout (INFO) are diverted into logcat. `ebc_log_fprintf` carries
`__attribute__((format(printf, 2, 3)))` — without it the macro would have silenced
`-Wformat` on all 83 vendored call sites, and a wrong conversion specifier in vendored code
is exactly what this tree must keep catching.

Six files include it: `librtlsdr.c` (36 calls), `convenience.c` (32), `tuner_r82xx.c` (7),
`tuner_e4k.c` (6), `tuner_fc0012.c` and `tuner_fc0013.c` (1 each). `libusb-andro/` does not
— libusb logs through `usbi_log`, which already lands in `__android_log_write`.

**Three symbols lost their `static`** in `librtlsdr.c` so the bridge can reach them:
`rtlsdr_set_if_freq()`, `fir_default` and `tuners`. Verified harmless at the boundary that
matters — none of them is exported from the linked `.so`, because the target sets
`-fvisibility=hidden` while `RTLSDR_API` marks the public API `visibility("default")`
explicitly (§5).

---

## 4. Building it

```cmake
add_subdirectory(ebc-sdr-native)
target_link_libraries(<app-target> PRIVATE ebc_sdr ${log-lib})
```

That is the whole integration. Include paths, compile options and the 16 KB page-alignment
link flags travel with the target — do not repeat them in the app, and do not list any file
of this tree in the app's own `add_library()`.

### What the target does

| | |
| --- | --- |
| PUBLIC includes | `rtl-sdr/include`, `libusb-andro/libusb` (rtlsdr433's `rtl433/src/sdr.c` includes `libusb.h` directly), `android` |
| PRIVATE includes | `rtl-sdr/src`, `libusb-andro`. **Load-bearing:** `librtlsdr_internal.h` exposes the private `struct rtlsdr_dev`, and PRIVATE is what stops anything outside the library from including it. |
| PRIVATE compile options | `-fvisibility=hidden`, `-funwind-tables`, `-fno-builtin-printf`, `-fno-builtin-fprintf` — Vorarbeit 4.5. These sat in AIS's global `CMAKE_C_FLAGS`, where they also hit its AIS decoder; rtlsdr433 and rtlsdrPager set none of them. |
| Strict warnings | only on `android/librtlsdr_andro.c`, including `-Werror=shorten-64-to-32` — the contract the five backported casts exist for. Vendored code is third-party and must not be "fixed" (AGENTS.md). |
| INTERFACE link options | 16 KB page alignment on arm64-v8a and x86_64 — Vorarbeit 4.6. INTERFACE because a static library has no link step; this way every `.so` that links `ebc_sdr` gets the guarantee by linking rather than by remembering two lines. Only AIS pinned them before. |
| PUBLIC definitions | `EBC_LOG_TAG` from the `EBC_SDR_LOG_TAG` cache variable |
| PUBLIC link libraries | the NDK `log` library |

### Options

| Option | Default | Meaning |
| --- | --- | --- |
| `EBC_SDR_CONVENIENCE` | `OFF` | Compile `rtl-sdr/src/convenience/convenience.c` into the library and export its include directory. Only `RTL_SDR_AIS_Driver` needs it. |
| `EBC_SDR_LOG_TAG` | `librtlsdr` | logcat tag for everything this library prints. |

`rtl-sdr/src/getopt/` is not vendored at all — no app compiles it, and Android has `getopt`
in libc.

### What each app had to do — Phases 2 to 4

**All three are migrated and all three pin tag `v0.3.0`:** `rtlsdrPager` (Phase 2, commit
`2743190`), `rtlsdr433` (Phase 3, commit `7f7d7cb`, released as v1.3.3) and
`RTL_SDR_AIS_Driver` (Phase 4, commit `4ce3a9d`, released as v1.4.0 / versionCode 56 on
2026-09-04). The list below is kept as the record of what each migration involved — and as
the checklist for any app that adopts this tree later.

1. **Map the error codes at the JNI boundary.** The library returns `EBC_SDR_ERR_LIBUSB_INIT`
   (-2001) and `EBC_SDR_ERR_CLAIM` (-2002), or a raw `LIBUSB_ERROR_*`. AIS's Java contract
   expects -50/-51, rtlsdr433 and rtlsdrPager -101/-102. Map, do not renumber the library.
   For `EBC_SDR_ERR_CLAIM`, ask `rtlsdr_last_open_was_busy()` before wording the message.
2. **`rtlsdr_open2()` takes two parameters.** AIS calls the 4-parameter form at
   `app/src/main/jni/rtl_ais_andro.c:1836`. rtlsdr433 additionally has a local prototype in
   the `__EBCANDROID__` block at `rtl433/src/sdr.c:38` that must move to
   `librtlsdr_andro.h`.
3. **Delete the app's own copies** of `rtl-sdr/`, `libusb-andro/` and `librtlsdr_andro.c/.h`,
   and remove their files from the app's `add_library()`. Leaving them in gives duplicate
   symbols now that `librtlsdr.c` is a real translation unit.
4. **Drop the flags the target now owns**: AIS's `-fvisibility=hidden`,
   `-fno-builtin-*printf`, `-funwind-tables` and the two `max-page-size` linker lines.
5. **Delete the dead `cpp/libusb_config.h`** in rtlsdr433 and rtlsdrPager. It carries a
   different, smaller definition set (no `ENABLE_LOGGING`, no `HAVE_LINUX_NETLINK_H`); if it
   were ever included, libusb behaviour would change silently.
6. **AIS keeps `aprintf_stderr` as a function** in `rtl_ais_andro.h` for its own sources.
   `ebc_log.h` defines it as a macro. The app must not include both in one translation unit,
   or rename its own.
7. **Verify on hardware.** Phase 2 is the rtlsdrPager pilot: open, tune across bands, switch
   gain mode, unplug while running, plug back in, restart the app.

---
## 5. Verification

All three apps were untouched throughout Phases 0 and 1: `git status --short` compared
before and after every step.

**Line endings.** The tree is LF-only, which is what makes a diff against a fresh upstream
clone mean anything:

```sh
python -c "import io,os,sys
bad=[os.path.join(d,f) for r in ('rtl-sdr','libusb-andro','android')
     for d,_,fs in os.walk(r) for f in fs
     if b'\r' in io.open(os.path.join(d,f),'rb').read()]
print(bad or 'clean')"
```

(`grep -rl $'\r'` is the obvious spelling but the `$'...'` quoting is easy to get wrong in a
wrapper shell, and an empty pattern silently matches every file.)

After the Phase 0 import, 47 of 49 files matched what the AIS repo *stores* — not what it
checks out, since `core.autocrlf=true` there converts to CRLF:

```sh
git -C ../RTL_SDR_AIS_Driver cat-file -p HEAD:app/src/main/jni/rtl-sdr/src/tuner_fc2580.c | md5sum
md5sum < rtl-sdr/src/tuner_fc2580.c
```

### The build

Phase 1 replaced the Phase 0 `-fsyntax-only` sweep with a real build of a real consumer.
NDK r29 (`29.0.14206865`), the version all three apps pin:

```sh
NDK=~/AppData/Local/Android/Sdk/ndk/29.0.14206865
CMAKE=~/AppData/Local/Android/Sdk/cmake/4.1.2/bin/cmake.exe
NINJA=~/AppData/Local/Android/Sdk/cmake/4.1.2/bin/ninja.exe

$CMAKE -S <consumer> -B build -G Ninja \
  -DCMAKE_MAKE_PROGRAM=$NINJA \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-23 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
$CMAKE --build build
```

The consumer is a small `.so` that does `add_subdirectory()` on this tree, links `ebc_sdr`,
and calls the surface an app actually calls — `rtlsdr_open2`, `rtlsdr_set_center_freq`,
`rtlsdr_set_sample_rate`, `rtlsdr_set_tuner_gain_mode`, `rtlsdr_is_dev_lost`,
`rtlsdr_last_open_was_busy`, `rtlsdr_cancel_async_save`, `rtlsdr_close` — plus an
`fprintf` of its own, to prove an app that does not include `ebc_log.h` keeps a real one.

**32 configurations built with 0 errors and 0 warnings:** 4 ABIs × API {23, 29} ×
{Debug, RelWithDebInfo} × `EBC_SDR_CONVENIENCE` {OFF, ON}. `libebc_sdr.a` holds 17 objects
(18 with convenience), and `libconsumer.so` links.

### What the build proves

| Check | Result |
| --- | --- |
| `librtlsdr.c` is a translation unit | It is listed in `add_library()` and the link succeeds — no duplicate symbols. That was impossible before Phase 1. |
| Public API exported | 44 `rtlsdr_*` symbols in `libconsumer.so`, on all four ABIs. |
| Internals hidden | 0 of `tuners`, `fir_default`, `rtlsdr_set_if_freq`, `get_string_descriptor`, `rtlsdr_init_baseband`, `ebc_log_fprintf` exported, on all four ABIs. So the three symbols that lost their `static` in §3.8 cost nothing at the boundary. |
| 16 KB page alignment | First `LOAD` segment aligned `0x4000` on arm64-v8a and x86_64, `0x1000` on armeabi-v7a and x86 — 64-bit only, as intended. Reached the consumer `.so` through `target_link_options(INTERFACE)`, which is the point. |
| `EBC_SDR_CONVENIENCE` is real | `convenience.c.o` is in the archive with `ON` and absent with `OFF`, and `verbose_device_search`/`nearest_gain`/`set_gain_by_perc` are in the `.a`. The `.so` is byte-identical either way because the consumer calls none of them and the linker drops unreferenced objects — correct static-library behaviour, not a broken option. |
| `fprintf` redirect complete | After preprocessing, **no** unredirected `fprintf(stderr` remains in any of the six files. Verified with `clang -E \| grep`. |
| API 23 floor holds | Every configuration above was also built at `android-23`, which is AIS's `minSdk`. |

```sh
# symbol checks
llvm-nm -D --defined-only libconsumer.so | grep -c ' T rtlsdr_'      # 44
llvm-nm -D --defined-only libconsumer.so | grep -cw tuners           # 0
llvm-readelf -l libconsumer.so | grep '^  LOAD'                      # align 0x4000
```

### Hardware verification

A green build proves very little here; the differences between the variants sit in hardware
behaviour. Three runs, one per migrated app — same dongle and same phone throughout.

#### First run: rtlsdrPager, Phase 2

**2026-09-04, first run on real hardware**, via rtlsdrPager at tag v0.1.0:

- **Device:** RTL-SDR Blog V4 (`0bda:2838`) on a Galaxy S25 FE, Android 16 / SDK 36,
  arm64-v8a. Android 16 matters: that is the kernel-6.12 platform whose
  `USBFS_CAP_REAP_AFTER_DISCONNECT` behaviour the libusb unplug fixes exist for.

| Check | Result |
| --- | --- |
| Library loads, device opens from the fd | `initNative OK`, `fd=141`, no `EBC_SDR_ERR_*` |
| Tuner probe | R828D found, `RTL-SDR Blog V4 Detected` |
| Tuning | 162.475 MHz and 439.9875 MHz, six opens |
| Gain | automatic and manual 39.8 dB, both applied |
| Streaming | 1.4112 MS/s, 12 x 262144-byte buffers, 5 111 808 samples in the longest run |
| End-to-end decode | POCSAG 1200 acquired sync, 36.6 % of in-sync words decoded |
| **Unplug while running** | `rtlsdr_read_async returned -5`, nine `cb transfer status: 5, canceling...`, then `the device was lost -- the dongle was unplugged` via `rtlsdr_is_dev_lost()`, clean teardown, `device closed`. **No SIGSEGV, no tombstone, no ANR; the process PID was unchanged across the whole session.** |
| Re-plug and restart | opened and streamed again without restarting the device |
| Befund 10, live | Three previously invisible messages in logcat, including `Exact sample rate is: 1411200.013458 Hz` straight out of `librtlsdr.c` |
| PLL | no `PLL not locked` on either band — see §6 |

**Not covered, and structurally not coverable by this app: Befund 5, the VGA gain reset per
tune.** That bug only bites when `r82xx_set_freq()` is called repeatedly on a *running*
device with AGC on. rtlsdrPager calls every `rtlsdr_set_*` from `configure_device()`, which
runs once per open — each frequency or gain change is a full stop/open cycle, so
`r82xx_set_gain()` runs fresh every time and the ~10 dB AGC loss never appears. That also
explains why this app never noticed the bug. The observable test belongs to whichever app
retunes a live device; check rtlsdr433 in Phase 3.

#### Second run: rtlsdr433, Phase 3

**2026-09-04, same afternoon, same dongle and phone**, via rtlsdr433 at tag v0.3.0 of this
tree — the first app to exercise it that carries a second GPL tree (`rtl_433`) reaching into
`rtl-sdr` and `libusb` itself.

| Check | Result |
| --- | --- |
| Device opens from the fd | `Opening RTL-SDR via Android USB fd=6`, no `EBC_SDR_ERR_*`, no `LIBUSB_ERROR_*` |
| Tuner probe | R828D found, `RTL-SDR Blog V4 Detected` — 8 opens across the session, all 8 identical |
| Tuning | three bands: 315.000, 433.920 and 868.300 MHz |
| Gain | automatic, and manual 20.0 dB requested → `Tuner gain set to 20.700000 dB` (nearest supported step) |
| Streaming | 250 kS/s and 1024 kS/s, 262144-byte buffers |
| End-to-end decode | two inFactory-TH sensors, CRC-checked, 28.8 °C / 45 % and 27.8 °C / 48 %, RSSI −7.7 and −13.1 dBm, SNR 15–19 |
| **Unplug while running** | ten `cb transfer status: 5, canceling...` (one per URB), then `LIBUSB_ERROR_NOT_FOUND` → `async read failed (-5)`, watchdog timeout 1.6 s later, `android_run_sdr_loop returned 3`, clean teardown. **No SIGSEGV, no tombstone, no ANR — the crash buffer held zero entries for the whole session and the PID was unchanged across the unplug.** |
| Re-plug | reopened on the first attempt with a fresh `fd=118`, no `LIBUSB_ERROR_BUSY`, 12 decodes afterwards |
| App restart without device restart | `am force-stop` then relaunch, new PID, opened again and produced 24 decodes |
| Befund 10, live | `Found Rafael Micro R828D tuner`, `RTL-SDR Blog V4 Detected` and `Exact sample rate is: 250000.000414 Hz` in logcat under the app's own tag `rtl433-sdr` |
| PLL | no `PLL not locked` on any of the three bands |
| 16 KB page alignment | `0x4000` on arm64-v8a and x86_64, checked in the packaged APK |

**Befund 5 is not observable in rtlsdr433 either, and the reason is worth recording.** The
app sets `cfg->frequencies = 1`, so rtl_433's frequency hopping never engages and
`rtlsdr_set_center_freq()` is called exactly once per open. Changing the band in the UI does
not retune the running device — the app says so itself ("Stop and start decoding to apply
it"), and the log confirms one `rtlsdr_set_center_freq` per session.

So two of the three apps cannot observe the VGA reset by construction, which retires the
question this section left open for Phase 3 rather than answering it. The fix stands on the
register analysis in §3.4: with AGC on, the old code nailed register `0x0c` to 16.3 dB while
the AGC path sets 26.5 dB. If a live-retune path is ever added to either app — rtl_433's own
hopping would be enough — the observable comes back with it.

The capture also showed the bridge logging tuner discovery at ERROR level, inherited from
rtlsdrPager's macro. Fixed afterwards: `aprintf_info()` for progress notes, `aprintf_stderr()`
kept for actual failures.

#### Third run: RTL_SDR_AIS_Driver, Phase 4

**2026-09-04, same dongle and phone**, via `RTL_SDR_AIS_Driver` at tag v0.3.0 of this tree —
the last of the three, and the only one with `minSdk 23`, a Java layer, `EBC_SDR_CONVENIENCE`
and an external API driven by a second app. Recorded in the app's own `agents.md`
("Hardware verification of this pin") and `CHANGELOG.md` for v1.4.0; reproduced here because
this tree's verification gate is a rule of this repository.

| Check | Result |
| --- | --- |
| Device opens from the fd | five opens (`fd` 127, 102, 126, 108 and AIS-Share's), no `EBC_SDR_ERR_*`, no `LIBUSB_ERROR_*` |
| Tuner probe | R828D found, `RTL-SDR Blog V4 Detected` — five times, identical |
| Tuning | two bands: 162.000 MHz (AIS) and 156.825 MHz (marine) |
| Gain | automatic, and manual 50 % with AGC off — the latter exercises `set_gain_by_perc()`, this project's own addition to `convenience.c` (§3.4) |
| Streaming | 3.13–3.20 MB/s steady, **`Skipped = 0` in every stats line** |
| End-to-end decode | two AIS messages on channel B — weak indoor reception, but RF to Java callback is proven |
| **Unplug while running** | ten `cb transfer status: 5, canceling...` (one per URB), `do_clean_up()` 21 ms later, done 146 ms after the first cancel — **7 ms *before* the framework broadcast `USB_DEVICE_DETACHED`.** No SIGSEGV, no tombstone, no ANR; PID unchanged |
| Re-plug | reopened on the first attempt with a fresh `fd`, no `LIBUSB_ERROR_BUSY` |
| App restart without device restart | `am force-stop` then relaunch: new PID, clean open, 3.19 MB/s |
| **External API via a second app** | `DeviceOpenActivity` launched from `eu.ebctech.ais_share`, driver started and streamed, status broadcasts flowed back; the `RTLSDRAIS_Exception` contract unchanged |
| Befund 10, live | five messages under the tag `ais-sdr` that were invisible before, including two at INFO via `aprintf_info()` and three from `convenience.c` through the `ebc_log.h` redirect |
| Whole session | 0 × SIGSEGV/SIGABRT/fdsan/tombstone, 0 ANR, 0 `LIBUSB_ERROR_*`, 0 `EBC_SDR_ERR_*`, 0 `PLL not locked`, crash buffer empty |

`armeabi-v7a` remains compile-verified only — no 32-bit ARM device was available. `Exact
sample rate is: …` did not appear here, correctly: 1 600 000 S/s is exactly representable.

**Befund 5 is not observable in this app either, for the same structural reason**: every
`rtlsdr_set_*` runs once per open, so there is no live retune. That makes it **all three
apps**, not two — the VGA-reset fix rests entirely on the register analysis in §3.4, and no
app in this family can currently show it in a log.

This run closed Phase 4. `RTL_SDR_AIS_Driver` released from it as v1.4.0 / versionCode 56 on
2026-09-04, so all three apps now ship the same shared base at the same tag — the state this
repository was built for.

---

### The syntax sweep still applies to the vendored sources

Useful on its own, because it isolates a file from the target's flags:

```sh
CLANG=$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe
INC="-Irtl-sdr/include -Irtl-sdr/src -Ilibusb-andro/libusb -Ilibusb-andro -Iandroid"
WARN="-Werror=shorten-64-to-32 -Werror=implicit-function-declaration -Werror=return-type"
for T in aarch64-linux-android23 armv7a-linux-androideabi21 x86_64-linux-android21 \
         i686-linux-android21; do
  for f in rtl-sdr/src/*.c rtl-sdr/src/convenience/convenience.c android/librtlsdr_andro.c \
           libusb-andro/libusb/*.c libusb-andro/libusb/os/*.c; do
    $CLANG --target=$T -fsyntax-only $INC $WARN "$f" || echo "FAIL $T $f"
  done
done
```

The same run against the **unpatched AIS tree** fails, which is what makes backport B1 worth
having:

```
tuner_e4k.c:448:14: error: implicit conversion loses integer precision:
        'uint64_t' to 'uint32_t' [-Werror,-Wshorten-64-to-32]
tuner_e4k.c:518:30: error: implicit conversion loses integer precision: ...
```

Remaining `-Wall` warnings (unused statics in `tuner_e4k.c`, `tuner_r82xx.c`, one misleading
indentation in `tuner_fc2580.c`) are **pre-existing upstream noise** — the counts match the
AIS original exactly, file by file. That is why the target puts `-Wall` on
`android/librtlsdr_andro.c` only.

**Dominance check.** Every difference between this tree and each of the three app trees is
either documented above, a deliberately discarded Blog hack (§3.3), Phase 1 library work
(§3.8), or app-specific glue:

```sh
for p in ../RTL_SDR_AIS_Driver/app/src/main/jni ../rtlsdr433/app/src/main/cpp \
         ../rtlsdrPager/app/src/main/cpp; do
  for f in $(find rtl-sdr libusb-andro -name '*.c' -o -name '*.h'); do
    diff --strip-trailing-cr -q "$p/$f" "$f" >/dev/null || echo "$p $f"
  done
done
```

Against the AIS tree the changed lines were exactly the patches in §3.1/§3.5/§3.6 at the end
of Phase 0 — verified line by line. §3.8 adds the library work on top.

**Dominance check.** Every difference between this tree and each of the three app trees is
either documented above, a deliberately discarded Blog hack (§3.3), or app-specific glue:

```sh
for p in ../RTL_SDR_AIS_Driver/app/src/main/jni ../rtlsdr433/app/src/main/cpp \
         ../rtlsdrPager/app/src/main/cpp; do
  for f in $(find rtl-sdr libusb-andro -name '*.c' -o -name '*.h'); do
    diff --strip-trailing-cr -q "$p/$f" "$f" >/dev/null || echo "$p $f"
  done
done
```

Against the AIS tree the changed lines are exactly the patches in §3.1/§3.5/§3.6 and nothing
else — verified line by line.

**Recreating the upstream clones** (they live in `tmp/`, which is `.gitignore`d):

```sh
git clone https://github.com/osmocom/rtl-sdr        tmp/osmocom-rtl-sdr
git clone https://github.com/rtlsdrblog/rtl-sdr-blog tmp/rtlsdrblog-rtl-sdr
git -C tmp/osmocom-rtl-sdr    checkout v2.0.3      # 797f814
git -C tmp/rtlsdrblog-rtl-sdr checkout V1.4.0      # aed0ea1
```

Note both clones check out **CRLF** on Windows (`core.autocrlf=true`), so always diff with
`--strip-trailing-cr`.

---

## 6. VCO current — decided on hardware

`r82xx_set_freq()` register `0x12`:

- **osmocom / this tree:** `0x80/0xe0` — VCO current "100" in bits 7:5, `pw_sdm` left alone.
- **Blog fork / rtlsdr433 / rtlsdrPager:** `0x06/0xff` — a different pattern over the full
  mask, which also clears `pw_sdm`.

The Blog's rationale was better PLL lock stability on the V4. The two are **not combinable**,
because the `0xff` mask overwrites bits osmocom sets deliberately. ANALYSE.md §6.2 therefore
set the criterion: keep the Blog variant only if reproducible lock dropouts are measured on a
V4.

### The measurement

**2026-09-04, RTL-SDR Blog V4** (`0bda:2838`, `product_name=Blog V4`, EEPROM manufacturer
`RTLSDRBlog`) on a Galaxy S25 FE, Android 16 / SDK 36, arm64-v8a, via rtlsdrPager at tag
v0.1.0 of this tree.

Six device opens across two bands — 162.475 MHz and 439.9875 MHz — with tuner gain both
automatic and manual (39.8 dB), streaming 1.4112 MS/s. **Neither**
`[R82XX] PLL not locked!` (`tuner_r82xx.c:573`) nor `[R82XX] No valid PLL values for %u Hz!`
(`:534`) appeared, and the POCSAG decoder acquired sync and decoded 36.6 % of in-sync words
at 1200 baud — so the PLL was not merely silent, it was delivering a usable signal.

**Decision: the osmocom value stays.** No lock dropouts, so the criterion for keeping the
Blog hack is not met.

Worth noting *why this was measurable at all*: those two PLL messages are
`fprintf(stderr, ...)` in `tuner_r82xx.c`. Before Befund 10 was fixed (§3.8) they went
nowhere on Android, so their absence proved nothing. Fixing the log redirect is what turned
this from an open question into an answered one.

Re-open it if lock dropouts ever show up on a V4 — the message will now be in logcat.

---
## 7. Candidates for an upstream patch

Findings that are in the upstreams too, and improvements that exist in all three EBC apps and
in neither upstream:

| To | What |
| --- | --- |
| osmocom + Blog | Befund 1 — `rtlsdr_open()` must check `rtlsdr_read_eeprom()`; deriving `force_bt` from an unwritten buffer can switch the bias tee on. |
| osmocom + Blog | Befund 2 — `get_string_descriptor()` reads `data[pos]` unbounded and can run ~260 bytes past the caller's stack buffer. |
| osmocom + Blog | Befund 4 — `set_gain_by_perc()` in `convenience.c` indexes a zero-byte allocation on tuners without a gain table. |
| osmocom + Blog | Befund 8 — NULL guard in `rtlsdr_set_direct_sampling()`. |
| osmocom + Blog | The resubmit return-value check in the transfer callback, which fixes the Android 16 / kernel 6.12 crash on unplug. Present in all three EBC apps, in neither upstream. |
| Blog | The four `rtlsdr_set_i2c_repeater(dev, 0)` calls the fork commented out. Both EBC variants had already reverted that. |

---

## 8. How to rebase onto a newer upstream

1. Clone the new upstream tag into `tmp/`.
2. `diff --strip-trailing-cr -u tmp/<upstream>/<file> rtl-sdr/<file>` per file. Every hunk must
   map to an entry in §3. **A hunk with no entry is a bug in this file, not in the code.**
3. Take the new upstream file, re-apply the §3 entries, run the §5 syntax check.
4. Update §1 with the new tag and commit, and note in [CHANGELOG.md](CHANGELOG.md) which
   entries in §3 became obsolete because upstream adopted them.
5. Verify on hardware before any app pins the new tag — a green build proves little here, the
   differences sit in hardware behaviour.
