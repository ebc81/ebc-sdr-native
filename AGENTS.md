# AGENTS.md — shared native SDR base (`ebc-sdr-native`)

Vendored `rtl-sdr` and `libusb-andro` for all EBC Android SDR apps: `RTL_SDR_AIS_Driver`,
`rtlsdr433`, `rtlsdrPager`.

**Read [PROVENANCE.md](PROVENANCE.md) before touching a `.c` or `.h` file in `rtl-sdr/`,
`libusb-andro/` or `android/`.** It lists every byte that differs from upstream and why.

---

## Take the union of fixes (MANDATORY)

This tree exists because three apps each carried their own copy and fixes flowed in one
direction only: rtlsdr433 found the Android-16 URB bug in May 2026 and `RTL_SDR_AIS_Driver`
took it over; rtlsdrPager corrected the lock ordering in August and AIS took that too. Only
rtlsdr433 got nothing back. That asymmetry is what this repository removes.

So: when two variants of the same code exist, **do not pick one — take the union.** If they
solve the same problem differently, decide which is technically better and record the reasoning
in PROVENANCE.md §3. If the answer depends on hardware you cannot measure, keep the upstream
value and write the open question into PROVENANCE.md §6 rather than guessing.

## Every deviation from upstream is documented (MANDATORY)

PROVENANCE.md is a contract, not a description. A hunk that does not map to an entry in §3 is
a bug in PROVENANCE.md, not something to shrug at.

- Change a vendored file → add or update the §3 entry **in the same commit**.
- Mark the site in the code with an `__EBCANDROID__` comment so `grep -rn __EBCANDROID__`
  finds every local deviation — 17 markers across 11 files today. In this tree that marker is
  a *comment convention*, not a compile guard, and Phase 1 decided to keep it that way: the
  library is Android-only, so there is no non-Android branch to select. (rtlsdrPager does use
  it as a real define, `-D__EBCANDROID__=1`, for its `multimon/` tree.) The one place that
  genuinely branches on the platform is `ebc_log.h`, and it uses the NDK's own `__ANDROID__`.
- One commit per logical patch. From this repository's first commit onward, `git log` and
  PROVENANCE.md must stay in agreement. The **Archive commit** hashes in PROVENANCE.md §3
  predate that and point into the private construction archive — they do not resolve here
  and are not supposed to.

## Line endings are LF (MANDATORY)

`.gitattributes` sets `* -text`, and every file in the tree is LF. Do not let Git for Windows'
`core.autocrlf=true` turn that into CRLF: the whole point is that
`diff --strip-trailing-cr -u tmp/<upstream>/<file> <file>` shows the real patches and nothing
else. Check it with the snippet in PROVENANCE.md §5 -- not with
`grep -rl $'\r' ...`, whose `$'...'` quoting is easy to lose in a wrapper shell, and
an empty pattern silently matches every file rather than none.

The upstream clones in `tmp/` *do* check out as CRLF, so always diff with
`--strip-trailing-cr`.

## A green build proves very little (MANDATORY)

The differences between the variants sit in hardware behaviour, not in compilation. Before any
app pins a tag, verify on a real device: open, tune across several bands, switch gain mode,
**unplug while running** (that is the path the libusb fixes are about), plug back in, restart
the app without restarting the device. For AIS also check against API 23 — it has `minSdk 23`
while the other two have 29.

The AGC/VGA finding is directly observable: change frequency repeatedly with AGC on. Before
(rtlsdr433/rtlsdrPager) the noise floor drops after every change; afterwards it does not.

---

## Current state

**Phases 0 to 4 are done.** The tree is the union of all three app variants, plus five
finding fixes, and it builds as a static library `ebc_sdr`. **All three apps use it**, each
pinning tag `v0.3.0` as a submodule, and each verified on a Blog V4 — PROVENANCE.md §5.
`RTL_SDR_AIS_Driver` was the last to migrate and released from that pin as v1.4.0 /
versionCode 56 on 2026-09-04; `rtlsdr433` ships it as v1.3.3. So the state this repository
was built for — one shared base, same tag, three shipping apps — is reached.

Phase 1 removed the blocker: `librtlsdr.c` used to be pulled into
`android/librtlsdr_andro.c` with `#include "rtl-sdr/src/librtlsdr.c"`, because the bridge needs
the private `struct rtlsdr_dev` — it assembles the device itself around
`libusb_wrap_sys_device(fd)` instead of calling `rtlsdr_open()`, since Android has no device
enumeration. `rtl-sdr/src/librtlsdr_internal.h` now exposes exactly that much, so
`librtlsdr.c` is a normal translation unit.

**Do not re-introduce that `#include`.** If the bridge needs something else from
`librtlsdr.c`, add it to `librtlsdr_internal.h` — moved verbatim, with a PROVENANCE.md §3.8
entry — rather than restructuring `librtlsdr.c`, which must stay close to upstream.

**Next: Phase 5**, the GPL source paths on the app side — see *Legal posture* below. Nothing
in this tree blocks it.

The three migrations are the template for any app that adopts this tree later, in the order
they happened: `rtlsdrPager` commit `2743190`, `rtlsdr433` commit `7f7d7cb`,
`RTL_SDR_AIS_Driver` commit `4ce3a9d`. AIS went last because it holds the most local
peculiarities: `minSdk 23`, a `-Werror` contract on three files, its own error-code range
(-50/-51 out of `rtlaisjava_err.h`), a Java rather than Kotlin layer, `aprintf_stderr` as a
function of its own in `rtl_ais_andro.h`, the four-parameter `rtlsdr_open2()` call it used to
make in `rtl_ais_andro.c`, and it is the only app that needs `EBC_SDR_CONVENIENCE`. What each
one had to change is listed in PROVENANCE.md §4.

## Layout

```
CMakeLists.txt  add_library(ebc_sdr STATIC ...)
rtl-sdr/        include/, src/, src/convenience/   GPL-2.0-or-later
libusb-andro/   libusb/, libusb/os/                LGPL-2.1-or-later
android/        librtlsdr_andro.c/.h, ebc_log.h    GPL-2.0-or-later (derives from librtlsdr.c)
tmp/            upstream clones, .gitignore'd, analysis only
```

`src/convenience/` is opt-in via `EBC_SDR_CONVENIENCE` — only `RTL_SDR_AIS_Driver` compiles it
today. `src/getopt/` was deliberately not imported; no app compiles it, and Android has
`getopt` in libc.

Two headers have no upstream and are ours: `rtl-sdr/src/librtlsdr_internal.h` (the private
interface of `librtlsdr.c`) and `android/ebc_log.h` (the `fprintf` redirect and
`aprintf_stderr`).

**`librtlsdr_internal.h` is not a public header.** It exposes `struct rtlsdr_dev`, whose layout
changes without notice. `rtl-sdr/src` is PRIVATE on the target precisely so nothing outside the
library can include it — do not make it PUBLIC to save an include path.

## Verification commands

Build it. The full 32-configuration matrix and the symbol and alignment checks are in
PROVENANCE.md §5; one configuration is enough while iterating:

```sh
NDK=~/AppData/Local/Android/Sdk/ndk/29.0.14206865
CMAKE=~/AppData/Local/Android/Sdk/cmake/4.1.2/bin/cmake.exe
NINJA=~/AppData/Local/Android/Sdk/cmake/4.1.2/bin/ninja.exe

$CMAKE -S . -B /tmp/b -G Ninja -DCMAKE_MAKE_PROGRAM=$NINJA \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-23 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
$CMAKE --build /tmp/b
```

Build at `android-23` at least once before calling anything done: that is
`RTL_SDR_AIS_Driver`'s `minSdk`, while the other two are at 29.

A single file, isolated from the target's flags:

```sh
CLANG=$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe
$CLANG --target=aarch64-linux-android23 -fsyntax-only \
  -Irtl-sdr/include -Irtl-sdr/src -Ilibusb-andro/libusb -Ilibusb-andro -Iandroid \
  -Werror=shorten-64-to-32 rtl-sdr/src/librtlsdr.c
```

`-Werror=shorten-64-to-32` is not optional: `RTL_SDR_AIS_Driver` applies it to its bridge, the
target applies it to ours, and it is the reason five narrowing casts were backported from
rtlsdr433. Removing them makes the build fail, which is the point.

Pre-existing `-Wall` noise (unused statics in `tuner_e4k.c` and `tuner_r82xx.c`, one misleading
indentation in `tuner_fc2580.c`) is upstream's and matches the AIS original file by file. That
is why the target puts `-Wall` on `android/librtlsdr_andro.c` only. Do not "fix" it in a
vendored file without an entry in PROVENANCE.md §3.

## Never touch the app repositories from here

Phases 0 and 1 changed nothing in `RTL_SDR_AIS_Driver`, `rtlsdr433` or `rtlsdrPager`, and
verified it by comparing `git status --short` before and after every step. **Keep doing that.**
The migrations (Phases 2 to 4) were the one sanctioned exception, and they are done. Work in
an app repository belongs in that app repository, from a session opened there.

When you do migrate an app, the app must **delete its own copies** of `rtl-sdr/`,
`libusb-andro/` and `librtlsdr_andro.c/.h` and remove them from its `add_library()`. Leaving
them in place gives duplicate symbols now that `librtlsdr.c` is a real translation unit.

## Legal posture

Two licences, kept in separate directories — see [LICENSE.md](LICENSE.md). The tree
effectively ships under the GPL because `android/` inlines `librtlsdr.c`.

This repository is **public** — `github.com/ebc81/ebc-sdr-native` — so for the shared part the
GPL source requirement is met structurally rather than by a copy step. That is the whole
argument for a shared repository over the per-app `-native-gpl` mirrors.

**Phase 5 is what is left, and it lives in the app repositories, not here:** each app's source
path has to name this repository and the tag it pins, and each existing mirror has to shrink to
the app-specific native code, with the submodule directory excluded from its sync. See
KONZEPT-GEMEINSAME-CODEBASE.md §2 and §3.4.
