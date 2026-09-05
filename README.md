# ebc-sdr-native

Shared native SDR base for the EBC Android SDR apps: vendored [osmocom/rtl-sdr][osmo] and
[libusb][libusb], plus the Android file-descriptor bridge that opens a dongle without device
enumeration. It builds as one static library target, `ebc_sdr`.

Three apps use it, each pinning the same tag as a git submodule: `RTL_SDR_AIS_Driver`,
`rtlsdr433` and `rtlsdrPager`.

[osmo]: https://github.com/osmocom/rtl-sdr
[libusb]: https://github.com/libusb/libusb

## Using it

Add it as a submodule under `app/src/main/cpp/ebc-sdr-native`, then in your `CMakeLists.txt`:

```cmake
add_subdirectory(ebc-sdr-native)
target_link_libraries(<app-target> PRIVATE ebc_sdr ${log-lib})
```

That is the whole integration. Include paths, compile options and the 16 KB page-alignment
linker flags travel with the target. Two CMake options exist: `EBC_SDR_CONVENIENCE` (default
`OFF`) compiles rtl-sdr's `convenience.c`, and `EBC_SDR_LOG_TAG` (default `librtlsdr`) sets the
logcat tag. API 23 is the supported floor.

Clone with `--recursive`, or run `git submodule update --init` in an existing checkout.

An app adopting this tree must **delete its own copies** of `rtl-sdr/`, `libusb-andro/` and
`librtlsdr_andro.c/.h` and drop them from its `add_library()` — otherwise the symbols are
defined twice.

## Layout

```
CMakeLists.txt  add_library(ebc_sdr STATIC ...)
rtl-sdr/        include/, src/, src/convenience/   GPL-2.0-or-later
libusb-andro/   libusb/, libusb/os/                LGPL-2.1-or-later
android/        librtlsdr_andro.c/.h, ebc_log.h    GPL-2.0-or-later
```

## Licensing

Two licences, kept in separate directories, neither relicensed — the details are in
[LICENSE.md](LICENSE.md). In practice **the whole tree ships under the GPL-2.0-or-later**,
because `android/` is a derivative work of `librtlsdr.c` and every consumer links `rtl-sdr/`
statically. This repository is public so that the GPL source requirement is met structurally
rather than by a copy step.

## Documentation

| File | What it is |
| --- | --- |
| [PROVENANCE.md](PROVENANCE.md) | The contract: upstream baselines and **every byte that differs from upstream, with its reason**. Read it before changing a vendored `.c` or `.h`. |
| [AGENTS.md](AGENTS.md) | Rules for working in this tree, the build and verification commands, and the current state. |
| [CHANGELOG.md](CHANGELOG.md) | Releases, including which entries change hardware behaviour. |
| [ANALYSE.md](ANALYSE.md) | The 2026-09-03 upstream comparison the tree was built from. Its numbered findings ("Befund 1"…"Befund 12") are cited from the source comments. |
| [KONZEPT-GEMEINSAME-CODEBASE.md](KONZEPT-GEMEINSAME-CODEBASE.md) | Why a shared repository, why a submodule, and the phase plan. |

A green build proves very little here — the differences between the merged variants sit in
hardware behaviour. Verify on a real device before pinning a new tag; AGENTS.md says how.
