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
  finds every local deviation — 30 markers across 11 files today. In this tree that marker is
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

**A consumer pins the gitlink, never a branch.** The pin that counts is the commit recorded in
the app's index — `git submodule status` must print the tag in parentheses, `(v0.4.0)`, not a
bare hash and not a branch head. Two things defeat that, and they combine: a `branch =` key in
the app's `.gitmodules`, and `git submodule update --remote`, which reads that key and moves
the pin to the tip of `main`. `main` here is routinely a documentation commit *ahead* of the
tag every app pins, so `--remote` silently swaps a verified tag for an untagged commit that no
hardware run covers. Use `git submodule update --init` in a consumer, and let a pin move only
as a deliberate, verified step.

---

## Current state

**Phases 0 to 5 are done.** The tree is the union of all three app variants, plus five
finding fixes, and it builds as a static library `ebc_sdr`. **All three apps use it**, each
pinning tag `v0.4.0` as a submodule, and each verified on a Blog V4 from that pin —
PROVENANCE.md §5, runs four to six. They ship it as `RTL_SDR_AIS_Driver` v1.4.1 /
versionCode 57 (since superseded by v1.4.2 / 58), `rtlsdr433` v1.3.4 and `rtlsdrPager` v1.5.1.

So the state this repository was built for — one shared base, same tag, three shipping apps —
is reached, and it has now survived the thing that actually tests it: a coordinated version
step. `v0.3.0` → `v0.4.0` moved all three apps, each with its own hardware run and its own
release, and nothing had to be duplicated or forked to do it. The earlier state, when all
three pinned `v0.3.0`, is the history recorded in CHANGELOG.md and PROVENANCE.md §5; on the
app side it spanned several releases per app, so no single version number names it.

Phase 1 removed the blocker: `librtlsdr.c` used to be pulled into
`android/librtlsdr_andro.c` with `#include "rtl-sdr/src/librtlsdr.c"`, because the bridge needs
the private `struct rtlsdr_dev` — it assembles the device itself around
`libusb_wrap_sys_device(fd)` instead of calling `rtlsdr_open()`, since Android has no device
enumeration. `rtl-sdr/src/librtlsdr_internal.h` now exposes exactly that much, so
`librtlsdr.c` is a normal translation unit.

**Do not re-introduce that `#include`.** If the bridge needs something else from
`librtlsdr.c`, add it to `librtlsdr_internal.h` — moved verbatim, with a PROVENANCE.md §3.8
entry — rather than restructuring `librtlsdr.c`, which must stay close to upstream.

**Phase 5 is done in all three app repositories** — the GPL source paths on the app side —
and it is now also **delivered**. The last gap was AIS: its corrected texts existed in the
repository but had not reached a binary, so the build on Play still carried the old page.
v1.4.1 / versionCode 57 shipped them, and v1.4.2 / 58 is current. A recipient of any of the
three binaries now gets a source route that names this repository and the tag that binary was
actually built from. The per-app state is in *Legal posture* below, which is the single place
this repository tracks it. Nothing here is outstanding.

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
effectively ships under the GPL because `android/` is built on GPL code moved verbatim out of
`librtlsdr.c` into `librtlsdr_internal.h`, and links `rtl-sdr/` statically.

This repository is **public** — `github.com/ebc81/ebc-sdr-native` — so for the shared part the
GPL source requirement is met structurally rather than by a copy step. That is the whole
argument for a shared repository over the per-app `-native-gpl` mirrors.

**Phase 5 lives in the app repositories, not here**, and in all three it is done. Each app's
source path has to name this repository and the tag it pins, and each existing mirror has to
shrink to the app-specific native code, with the submodule directory excluded from its sync.
State on 2026-09-13, read in the app repositories. All three now name tag **`v0.4.0`**, which
is the tag their current binaries were built from — that congruence is the whole point, and it
is what a pin bump has to carry along:

| App | Phase 5 |
| --- | --- |
| `rtlsdr433` | **done and shipped.** `NOTICE` names this repository and tag `v0.4.0` with a three-year written offer; `tools/sync-native-gpl.ps1` excludes the submodule with `/XD ebc-sdr-native`; the mirror is synced and shrunk. The tag in `NOTICE` moved with the pin at v1.3.4. |
| `rtlsdrPager` | **done.** Same `NOTICE` and the same script, now at tag `v0.4.0`; it is the only one of the three that also carries the tag in a machine-readable place, `app/config/libraries/ebcsdrnative.json`, which is the copy the in-app licence screen shows. Its mirror was synced and shrunk with v1.1.2 (`23dde9a`) — the written offer names both repositories — and v1.2.0 (`62a4b24`) tags the mirror alongside the app. Its Play state is not tracked here; it was unpublished when this table was first written. |
| `RTL_SDR_AIS_Driver` | **done and shipped.** It has no mirror and needs none — the app is GPL as a whole, so what it owes is a written offer over the *entire* app source, and `9f9d9c6` widened the offer to exactly that, added the three-year term and an entry for this repository, in a new root `NOTICE` and in the in-app page (`assets/open_source_licenses.html`) that is the copy a binary recipient actually gets. `92e6028` made the GPLv2 text reachable there at all. Both now name tag `v0.4.0`. Those texts were written without a version bump and waited for a release; **v1.4.1 / versionCode 57 delivered them to Play**, and v1.4.2 / 58 is current. The gap where Play still served the old page is closed. |

See KONZEPT-GEMEINSAME-CODEBASE.md §2 and §3.4. Fix any of it **from a session opened in that
app repository**, not from here.
