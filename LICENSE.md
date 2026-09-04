# Licensing

This repository holds code under **two different licences**. They are kept in separate
directories, each with its own licence text, and neither is relicensed.

| Directory | Licence | Full text | Upstream |
| --- | --- | --- | --- |
| `rtl-sdr/` | GNU **GPL-2.0-or-later** | [`rtl-sdr/COPYING`](rtl-sdr/COPYING) | [osmocom/rtl-sdr](https://github.com/osmocom/rtl-sdr), with features from [rtlsdrblog/rtl-sdr-blog](https://github.com/rtlsdrblog/rtl-sdr-blog) |
| `libusb-andro/` | GNU **LGPL-2.1-or-later** | [`libusb-andro/COPYING`](libusb-andro/COPYING) | [libusb/libusb](https://github.com/libusb/libusb) 1.0.23 |
| `android/` | GNU **GPL-2.0-or-later** | [`rtl-sdr/COPYING`](rtl-sdr/COPYING) | EBC. It `#include`s `rtl-sdr/src/librtlsdr.c` directly, so it is a derivative work of GPL code. |

Every source file carries its own licence header. Those headers are authoritative; this file
only summarises them.

Exact provenance of every local change is in [PROVENANCE.md](PROVENANCE.md).

## Practical consequences

**The whole tree effectively ships under the GPL.** `android/librtlsdr_andro.c` inlines
`librtlsdr.c`, and any app linking the result links GPL code, so the app's binary is covered
by the GPL-2.0-or-later. The LGPL on `libusb-andro/` does not weaken that — it only means
libusb itself could be used under weaker terms in a different project.

**Source must be published alongside every release.** This repository is intended to be public
from the start, precisely so the GPL source requirement is met structurally instead of by a
copy step (KONZEPT-GEMEINSAME-CODEBASE.md §2). Each app that pins a tag of this repository
points its own `NOTICE` written offer at that tag.

## Attribution

`rtl-sdr` is Copyright © Steve Markgraf, Dimitri Stolnikov, Hoernchen, Kyle Keen and other
contributors; the V4/V4L support originates with RTL-SDR Blog. `libusb` is Copyright © Daniel
Drake, Johannes Erdfelt, Nathan Hjelm, Hans de Goede and other contributors. The
file-descriptor open path in `android/` follows Martin Marinov's `librtlsdr_andro` (2012).
Local modifications are Copyright © Christian Ebner.
