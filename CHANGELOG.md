# Changelog

Shared native SDR base for the EBC Android apps. Every entry that changes hardware behaviour
says so explicitly, because a green build proves very little here — the differences sit in what
the dongle does.

Details and reasoning for every item: [PROVENANCE.md](PROVENANCE.md).

## v0.3.0 — PPM search removed (2026-09-04)

**The tag all three apps pin today:** `rtlsdrPager`, `rtlsdr433` (released as v1.3.3) and
`RTL_SDR_AIS_Driver` (released as v1.4.0 / versionCode 56 on 2026-09-04). Each one is verified
on a Blog V4 — PROVENANCE.md §5.

### Removed

- **`rtlsdr_supporting_ppm_search()`.** Gone from `librtlsdr_andro.c` and the public
  header. It had no caller anywhere: not in native code, and not across JNI either —
  `RTL_SDR_AIS_Driver`'s `support_ppm_search()` JNI method returns a hardcoded `JNI_FALSE`
  and never called it. The function only ever returned 1.

  Phase 1 had kept it (PROVENANCE.md P9) because dropping a published `RTLSDR_API` symbol
  while all three apps still linked their own copies bought nothing. rtlsdrPager is
  migrated now, and the PPM-search subsystem this belonged to is being removed from
  `RTL_SDR_AIS_Driver` in the same pass, so the reason to keep it is gone.

  *Migration:* nothing to do. If some caller does appear, it was getting a constant 1 —
  a capability flag for a feature that has been compiled out of AIS for a long time
  (`SEARCH_PPM_FUNC 0`).

---

## Phase 1, the tree is a library (2026-09-04) — shipped in v0.3.0

Written while the phase was current, when no app used the tree yet. All three do now, each
pinning `v0.3.0`; PROVENANCE.md §4 records what each migration involved.

### Added

- **`CMakeLists.txt` with a real target.** `add_subdirectory(ebc-sdr-native)` plus
  `target_link_libraries(<app> PRIVATE ebc_sdr ${log-lib})` is the whole integration.
  Include paths, compile options and linker flags travel with the target.
- `EBC_SDR_CONVENIENCE` (default `OFF`) makes rtl-sdr's `convenience.c` an opt-in module.
  Only `RTL_SDR_AIS_Driver` compiles it today.
- `EBC_SDR_LOG_TAG` (default `librtlsdr`) sets the logcat tag for everything the library
  prints.
- **16 KB page alignment for every consumer.** The two `max-page-size` linker flags are
  `target_link_options(INTERFACE)`, so any `.so` linking `ebc_sdr` gets Android 15
  compliance on 64-bit devices by linking rather than by remembering. Only
  `RTL_SDR_AIS_Driver` pinned them before; rtlsdr433 and rtlsdrPager relied on the NDK r29
  default, which an NDK downgrade below r28 would have broken on a green build.

### Fixed

- **The library's diagnostics reach logcat — Befund 10 is now fully closed.** All 83
  `fprintf(stderr, ...)` calls in rtl-sdr (tuner detection, PLL failures, exact sample rate,
  "No supported tuner found") went nowhere on Android. `android/ebc_log.h` redirects
  `fprintf` in the six affected files, leaving the call sites untouched so the vendored code
  stays diffable against upstream. The shim inspects the stream, so a genuine `fprintf` to a
  file still goes to that file; only stderr (WARN) and stdout (INFO) are diverted.
  `-Wformat` stays sharp on all 83 sites via `__attribute__((format(printf, 2, 3)))`.
- **Befund 11.** `rtlsdr_supporting_ppm_search()` was declared in all three apps' headers
  and defined only in AIS's. One bridge, one definition, all three see it. (The function has
  no caller anywhere and always returns 1; kept for now because it is `RTLSDR_API`.)
- `libusb-andro/libusb/config.h` removed. Two `config.h` on the include path was a trap even
  though the two files were semantically identical — AIS resolved `<config.h>` to one,
  rtlsdr433 and rtlsdrPager to the other.

### Changed — API

- **`rtlsdr_open2()` takes two parameters:** `rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd)`.
  AIS's 4-parameter form carried `index` and `uspfs_path` from Martin Marinov's 2012 wrapper;
  both have been ignored ever since `libusb_wrap_sys_device()` replaced device lookup.
  rtlsdr433 and rtlsdrPager already used this form.

  *Migration:* AIS calls the old form at `app/src/main/jni/rtl_ais_andro.c:1836`. rtlsdr433
  also has a local prototype in the `__EBCANDROID__` block at `rtl433/src/sdr.c:38`.

- **Own error codes, `EBC_SDR_ERR_*` from -2000.** `rtlsdr_open2()` returns
  `EBC_SDR_ERR_LIBUSB_INIT` (-2001) or `EBC_SDR_ERR_CLAIM` (-2002) where it used to return
  each app's own numbers, or still a raw `LIBUSB_ERROR_*`. The range is far from everything
  already in play, so a caller can tell by range alone where a code came from.

  *Migration:* map at the JNI boundary — AIS's Java contract expects -50/-51, rtlsdr433 and
  rtlsdrPager -101/-102. Do not renumber the library. The mapping table is in
  `librtlsdr_andro.h`.

- `aprintf_stderr` is a macro over `__android_log_print`, not a function. AIS declared it as
  a function in `rtl_ais_andro.h`; that app must not include both in one translation unit.

### Internal

- **`librtlsdr.c` is a translation unit.** This was the Phase 1 blocker:
  `android/librtlsdr_andro.c` pulled the whole implementation in with `#include`, because it
  needs the private `struct rtlsdr_dev` — on Android there is no enumeration, so
  `rtlsdr_open2()` assembles the device itself around `libusb_wrap_sys_device(fd)`. That made
  a CMake target impossible (duplicate symbols) and froze the relative layout of the two
  trees. New internal header `rtl-sdr/src/librtlsdr_internal.h` carries exactly what the
  bridge needs, moved verbatim.
- Three symbols lost their `static` for that: `rtlsdr_set_if_freq()`, `fir_default`,
  `tuners`. None is exported from the linked `.so` — verified on all four ABIs.
- `android/librtlsdr_andro.c` compiles without any app header for the first time.
- Compile flags moved from AIS's global `CMAKE_C_FLAGS` onto the target, where they no longer
  reach that app's own sources and its AIS decoder. Strict warnings, including
  `-Werror=shorten-64-to-32`, apply to the Android glue only — vendored code is third-party.

### Verified

- **32 configurations built, 0 errors, 0 warnings:** 4 ABIs × API {23, 29} ×
  {Debug, RelWithDebInfo} × `EBC_SDR_CONVENIENCE` {OFF, ON}, NDK r29. A minimal consumer
  `.so` links `libebc_sdr.a` and calls the surface an app calls.
- 44 `rtlsdr_*` symbols exported, 0 internals, on all four ABIs.
- First `LOAD` segment aligned `0x4000` on arm64-v8a and x86_64, `0x1000` on the 32-bit
  ABIs — 64-bit only, as intended.
- No unredirected `fprintf(stderr` left in any of the six files after preprocessing.
- All three app repositories still untouched at this point.
- No hardware verification yet at this point. It started in Phase 2 with the rtlsdrPager
  pilot; all three runs are in PROVENANCE.md §5.

---

## Phase 0, union tree (2026-09-04) — shipped in v0.3.0

At the end of this phase the tree was still not buildable as a library — that came with
Phase 1 above.

### Base

- Imported `RTL_SDR_AIS_Driver`'s native tree byte-for-byte as the union base: osmocom
  rtl-sdr v2.0.3 (`797f814`) plus the worthwhile RTL-SDR-Blog features, and libusb 1.0.23
  (`LIBUSB_NANO 11397`) with EBC's Android unplug fixes.
- Normalised the tree to LF and pinned it with `.gitattributes` `* -text`, so a diff against a
  fresh upstream clone is meaningful.
- Left out `src/getopt/` (compiled by no app) and rtlsdr433's two `.bak` files.

### Fixed

- **Bias tee could switch on by itself (severity high).** `rtlsdr_open()` ignored the return
  value of `rtlsdr_read_eeprom()`. After a read error `buf[7]` came from uninitialised stack,
  so `force_bt` could become 1 and put **5 V on the antenna connector** of a device whose
  EEPROM never asked for it. Affects `RTL_SDR_AIS_Driver` and the Blog upstream. Befund 1.
- NULL guard in `rtlsdr_set_direct_sampling()`, which dereferenced `dev` before checking it.
  Befund 8.
- Five narrowing casts in `librtlsdr.c` and `tuner_e4k.c`. Without them the tree does not
  compile under `-Werror=shorten-64-to-32` — verified, not theoretical.
- In `tuner_fc0013.c` a local comment claimed the opposite of the register write it described;
  restored the upstream wording. Code unchanged.

### Changed — behaviour

- **`rtlsdr_set_offset_tuning()` no longer switches the bias tee.** On R820T/R828D it now just
  returns `-2` ("not supported"), which is osmocom behaviour and what rtlsdr433/rtlsdrPager
  already did. A call that answers "not supported" must not have a 5 V side effect.

  *Migration for `RTL_SDR_AIS_Driver`:* anyone who used the offset-tuning control to switch the
  bias tee needs a real bias-tee control calling `rtlsdr_set_bias_tee()`. **Settled in Phase 4:
  no caller in that app ever used offset tuning, so nothing had to be added.**

- **rtlsdr433 and rtlsdrPager will change R82xx behaviour when they adopt this tree.** The tree
  follows osmocom, not the Blog fork:
  - *no VGA gain re-set on every tune* — a fix, not a loss: their current code pinned register
    `0x0c` to 16.3 dB while AGC mode wants 26.5 dB, so **AGC lost ~10 dB on every frequency
    change.** Visible directly: change frequency repeatedly with AGC on and watch the noise
    floor settle instead of dropping. Befund 5.
  - *FC0013 register 0x14 becomes `0x40`* instead of `0x20` — the correct band switch. Affects
    FC0013 dongles only. Befund 6.
  - *no L-band dropout tweak* (`div_buf_cur = 0xa0`) — only acts above ~1 GHz, irrelevant for
    433 MHz ISM and POCSAG.
  - *VCO current stays at the osmocom value* `0x80/0xe0` instead of the Blog maximum
    `0x06/0xff`. The two are not combinable. Open at the time, **since decided on a V4: the
    osmocom value stays** — PROVENANCE.md §6.
  - They also gain the libusb unplug fixes they are missing today (Befund 3, severity high:
    use-after-free when unplugging on Android 14/15) and the hardened `get_string_descriptor()`
    (Befund 2) and `set_gain_by_perc()` (Befund 4).

### Added

- `rtlsdr_last_open_was_busy()` — tells "interface still claimed" (`LIBUSB_ERROR_BUSY` right
  after a re-plug; waiting helps) apart from a broken dongle. From rtlsdrPager.
- `rtlsdr_is_dev_lost()` — an unplug is otherwise indistinguishable from a read error, because
  `dev_lost` sits in the private `struct rtlsdr_dev`. From rtlsdrPager.

### Removed

- The unused `<android/log.h>` include in `librtlsdr.c`. The other half of Befund 10 — 83
  `fprintf(stderr, ...)` calls that go nowhere on Android — is **still open** and deferred to
  Phase 1, because a redirect needs a shared header and a real build to verify. The site is
  marked `__EBCANDROID__`.
- The unused `uint32_t rf_freq` field from `struct r82xx_priv`.
- The declaration of `r82xx_set_vga_gain()`, together with the hack that used it.

### Documentation

- [PROVENANCE.md](PROVENANCE.md) — upstream baselines, every local patch with its reason, the
  Phase 1 blockers, the verification commands, the open VCO question and the list of candidates
  for an upstream patch.
- [LICENSE.md](LICENSE.md) — GPL-2.0-or-later for `rtl-sdr/` and `android/`, LGPL-2.1-or-later
  for `libusb-andro/`, kept separate.
- [AGENTS.md](AGENTS.md) — the rules for working in this tree.

### Verified

- 17 translation units syntax-checked with NDK r29 across all four ABIs and against API 23
  (85 compilations), with
  `-Werror=shorten-64-to-32`: **0 errors.** The same run against the unpatched AIS tree fails
  at the two `tuner_e4k.c` sites.
- Every difference from each of the three app trees classified: documented patch, deliberately
  discarded Blog hack, or app-specific glue. Nothing missing from any app.
- All three app repositories untouched — `git status --short` compared before and after every
  step.
- No hardware verification yet. That starts in Phase 2 with the rtlsdrPager pilot.
