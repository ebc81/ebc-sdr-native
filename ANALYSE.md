# rtl-sdr / libusb-andro — Bestandsaufnahme und Upstream-Vergleich

**Erstellt:** 2026-09-03
**Umfang:** Analyse der einvendorten `rtl-sdr`- und `libusb-andro`-Bäume in
`RTL_SDR_AIS_Driver`, `rtlsdr433` und `rtlsdrPager`, verglichen mit den beiden
relevanten Upstreams.
**Status:** Reine Analyse, Momentaufnahme vom 2026-09-03. An den drei App-Projekten wurde
zu diesem Zeitpunkt nichts verändert; inzwischen sind alle drei migriert. Wo dieses Dokument
etwas als offen bezeichnet, gilt [PROVENANCE.md](PROVENANCE.md) — dort steht der Stand.

## Untersuchte Stände

| Quelle | Stand | Commit | Datum |
| --- | --- | --- | --- |
| osmocom/rtl-sdr | Tag `v2.0.3` (letztes Release) | `797f814` | 2026-08-12 |
| rtlsdrblog/rtl-sdr-blog | Tag `V1.4.0` (letztes Release, = master) | `aed0ea1` | 2026-03-22 |
| RTL_SDR_AIS_Driver | Arbeitsverzeichnis, Tag `v1.3.20` + 1 Commit | `b65463c` | 2026-08-23 |
| rtlsdr433 | Arbeitsverzeichnis, Tag `v1.3.2` | — | 2026-08-20 |
| rtlsdrPager | Arbeitsverzeichnis, Tag `v1.1.1` | `81d69c0` | 2026-08-23 |

Die Upstream-Clones liegen unter `libs_ebc\tmp\osmocom-rtl-sdr` und
`libs_ebc\tmp\rtlsdrblog-rtl-sdr`.

**Scope:** libusb wurde nicht gegen Upstream verglichen (kein Clone von `libusb/libusb`,
keine 1.0.29-Upgrade-Bewertung). Der Drei-Wege-Vergleich der App-eigenen libusb-Kopien
ist enthalten, weil er ohne Zusatzaufwand vorlag.

---

## 1. Kernbefund

Drei Aussagen, die alles Weitere bestimmen.

### 1.1 Es gibt nur zwei rtl-sdr-Varianten, nicht drei

`rtlsdr433` und `rtlsdrPager` sind bei `rtl-sdr/` **byte-identisch** — Pager hat den Baum
im August 2026 komplett von 433 übernommen und seither nicht angefasst. Nur AIS weicht ab.

Bei `libusb-andro/` sind 27 von 28 Dateien in allen drei Apps byte-identisch; einzig
`libusb/os/linux_usbfs.c` ist dreifach divergent — und dort sind AIS und Pager funktional
gleich, nur 433 hängt zurück.

Die Aufgabe ist damit deutlich kleiner als sie aussieht: es sind zwei Bäume zu vereinen,
nicht drei.

### 1.2 osmocom v2.0.3 ist bereits „das Beste aus beiden Welten"

Das war die offene Frage — sie hat eine überraschend klare Antwort.

osmocom mainline hat den RTL-SDR-Blog-**V4- und V4L-Support inzwischen vollständig
übernommen**: Erkennung über `rtlsdr_check_dongle_model()`, 28,8-MHz-Auto-Upconvert für
HF, Notch-Filter-Steuerung, HF-Tracking-Filter-Bypass, GPIO5-Upconverter-Umschaltung,
`vco_power_ref = 1` für V4L, HF/VHF/UHF-Bandumschaltung über Cable1/Cable2/Air-In.

Gleichzeitig hat osmocom Verbesserungen, die der Blog-Fork **nicht** hat:

- `shadow_equal()` — überspringt Registerschreibvorgänge, deren Wert schon anliegt
- gebündelter Blockwrite der Register 0x10–0x16 statt sechs Einzeltransaktionen
- exakte Fixpunkt-PLL (`vco_div = (pll_ref + 65536 * vco_freq) / (2 * pll_ref)`) statt
  der iterativen SDM-Schleife mit Fehlerakkumulation
- `memset(priv->regs, 0, NUM_REGS)` vor dem Init-Write
- `shadow_store()` mit korrektem `val -= r` bei negativem Offset

**Ein Wechsel auf osmocom v2.0.3 kostet also keinen V4/V4L-Support.** Verloren gehen nur
vier Blog-*Hacks*, die einzeln zu bewerten sind (Abschnitt 6.2).

### 1.3 AIS ist bereits fast dort

AIS' `tuner_r82xx.c` und `librtlsdr.c` sind osmocom v2.0.3 plus Android-Anpassungen plus
zwei bewusst behaltene Blog-Hacks. 433/Pager sitzen dagegen auf dem älteren Blog-Fork.

Die gemeinsame Codebase sollte deshalb von der **AIS-Variante** ausgehen, nicht von der
433/Pager-Variante — mit gezielten Rückportierungen aus 433 (Abschnitt 8).

---

## 2. Datei-Identität

MD5 über alle gemeinsamen Dateien der drei Apps.

### libusb-andro (28 Dateien, libusb 1.0.23 / `LIBUSB_NANO 11397`)

| Status | Dateien |
| --- | --- |
| in allen drei identisch | 27 — u.a. `core.c`, `io.c`, `sync.c`, `descriptor.c`, `hotplug.c`, `libusbi.h`, `strerror.c`, `os/linux_netlink.c`, `os/poll_posix.c`, `os/threads_posix.c`, `libusb.h`, beide `config.h` |
| dreifach divergent | **1** — `libusb/os/linux_usbfs.c` |

### rtl-sdr (22 Dateien, in 433 zusätzlich 2 `.bak`)

| Status | Dateien |
| --- | --- |
| in allen drei identisch | 14 — u.a. `tuner_fc2580.c`, `reg_field.h`, `rtlsdr_i2c.h`, `rtl-sdr_export.h`, `convenience.h`, `getopt/*` |
| 433 ≡ Pager, AIS abweichend | 8 — `src/librtlsdr.c`, `src/tuner_r82xx.c`, `src/tuner_e4k.c`, `src/tuner_fc0012.c`, `src/tuner_fc0013.c`, `src/convenience/convenience.c`, `include/rtl-sdr.h`, `include/tuner_r82xx.h` |
| nur in 433 | 2 — `librtlsdr.c.bak`, `tuner_r82xx.c.bak` (Stand 2023-10-30, unmodifizierter Blog-Code; als Merge-Base-Referenz brauchbar, gehören sonst nicht ins Repo) |

Die öffentliche `RTLSDR_API`-Oberfläche ist in allen drei Apps **identisch**. Eine
gemeinsame Bibliothek ist ohne API-Bruch machbar.

### Zeilenzahlen im Fünf-Wege-Vergleich

| Datei | AIS | 433 | Pager | osmocom 2.0.3 | blog 1.4.0 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `src/librtlsdr.c` | 2160 | 2130 | 2130 | 2057 | 2114 |
| `src/tuner_r82xx.c` | 1394 | 1413 | 1413 | 1427 | 1460 |
| `src/tuner_fc0012.c` | 335 | 345 | 345 | 333 | 345 |
| `src/convenience/convenience.c` | 354 | 338 | 338 | 308 | 308 |
| `include/rtl-sdr.h` | 416 | 429 | 429 | 407 | 416 |

### Nähe zum jeweiligen Upstream (Anzahl geänderter Zeilen)

| Datei | AIS↔osmocom | AIS↔blog | 433↔osmocom | 433↔blog | osmocom↔blog |
| --- | ---: | ---: | ---: | ---: | ---: |
| `librtlsdr.c` | 145 | 126 | 141 | 160 | 85 |
| `tuner_r82xx.c` | **137** | 238 | 150 | **103** | 171 |
| `tuner_e4k.c` | **0** | **0** | 4 | 4 | 0 |
| `tuner_fc0012.c` | **2** | 26 | 24 | **0** | 24 |
| `rtl-sdr.h` | 9 | **0** | 22 | 23 | 9 |
| `convenience.c` | 62 | 62 | 46 | 46 | **0** |

Die Diagonale ist eindeutig: **AIS' R82xx-Baum folgt osmocom, der von 433/Pager folgt dem
Blog-Fork.** AIS' `tuner_e4k.c` ist mit beiden Upstreams identisch, sein `tuner_fc0012.c`
praktisch osmocom, sein `rtl-sdr.h` exakt der Blog-Header.

---

## 3. Vergleichsmatrix

Legende: **✔** vorhanden/aktuell · **○** vorhanden, aber älter oder abweichend · **✘** fehlt

| Bereich | AIS | 433 | Pager | osmocom v2.0.3 | blog V1.4.0 | Bewertung |
| --- | --- | --- | --- | --- | --- | --- |
| **Öffentliche API** (`rtl-sdr.h`) | ✔ = blog | ○ umformatiert | ○ = 433 | ○ ohne `rtlsdr_ds_mode` / `check_dongle_model` | ✔ Referenz | Alle drei Apps API-kompatibel. osmocom deklariert `rtlsdr_check_dongle_model` nicht öffentlich — beim Umstieg nachzutragen. |
| **`librtlsdr.c` Basis** | osmocom 2.0.3 + Blog-Features | blog 1.4.0 | = 433 | Referenz | Referenz | AIS ist die modernere Basis. |
| **Device-Enumeration** | umgangen (fd) | umgangen (fd) | umgangen (fd) | `libusb_get_device_list` | dito | Auf Android läuft der Open ausschließlich über `librtlsdr_andro.c` + `libusb_wrap_sys_device(fd)`; `rtlsdr_open()` ist toter Code. |
| **Device-Init / Tuner-Probe** | im Bridge dupliziert, gehärtet | im Bridge dupliziert, ungehärtet | = 433 | in `rtlsdr_open` | dito | AIS prüft EEPROM-Rückgabe und Tuner-Init-Fehler, 433/Pager nicht. |
| **Reset** | Dummy-Write → `libusb_reset_device` | dito | dito | dito | dito | identisch |
| **Async-API / USB-Transfers** | ✔ + Resubmit-Check | ✔ + Resubmit-Check | ✔ + Resubmit-Check | ✘ kein Resubmit-Check | ✘ | **Alle drei Apps sind hier besser als beide Upstreams.** Kandidat für einen Upstream-Patch. |
| **`dev_lost`-Behandlung** | ✔ Flag + Loop-Check | ○ nur `cancel_async` im Callback | ○ = 433 | ✔ Flag + Loop-Check | ✘ entfernt | AIS/osmocom sind korrekt: kein `libusb_cancel_transfer` aus dem Callback heraus. |
| **Buffering** | 15 × 262144 B | identisch | identisch | identisch | identisch | keine Divergenz |
| **Threading** | unverändert | unverändert | unverändert | Referenz | Referenz | Locking liegt komplett im App-Layer. |
| **`i2c_repeater` aus nach Tuner-Op** | ✔ (4×) | ✔ (4×) | ✔ (4×) | ✔ | ✘ auskommentiert | Beide Apps haben die Blog-Regression bereits rückgängig gemacht. |
| **Frequenz / Direct Sampling** | ✔ Auto-DS + `_rtlsdr_set_direct_sampling` | ✔ + `!dev`-Guard | ✔ = 433 | ✘ kein Auto-DS | ✔ Ursprung | Blog-Feature, das osmocom fehlt — beim Umstieg mitzunehmen. |
| **Sample Rate** | ohne Casts | mit `(uint32_t)`-Cast | = 433 | ohne Casts | ohne Casts | Casts sind 433-eigen und sinnvoll (5.2). |
| **Gain R82xx** | osmocom-Verhalten | Blog + VGA-Reset pro Tune | = 433 | Referenz | Blog | **Funktional relevant** — siehe 5.1. |
| **Gain FC0012** | ✔ osmocom-Interpolation | ○ alte `switch`-Tabelle | ○ = 433 | ✔ | ○ | osmocom interpoliert über Bereiche, Blog kennt nur exakte Werte. |
| **Gain FC0013** | ✔ `0x40` | ✘ `0x20` (Zeile 255) | ✘ = 433 | ✔ `0x40` | ✔ `0x40` | **Abweichung von allen Upstreams** — siehe 5.4. |
| **AGC** | identisch | identisch | identisch | Referenz | Referenz | keine Divergenz |
| **Offset Tuning** | ○ Blog-Hack (schaltet Bias-T) | ✔ sauber `-2` | ✔ = 433 | ✔ sauber `-2` | ○ Blog-Hack | 433/Pager entsprechen osmocom. AIS' Verhalten ist ein Hack mit Nebenwirkung. |
| **Bias-T / `force_bt`** | ✔ aktiv (EEPROM Byte 7) | ○ Feld existiert, wird nie gesetzt | ○ = 433 | ✘ | ✔ Ursprung | AIS erbt dabei einen Upstream-Bug (5.3). |
| **GPIO** | identisch | identisch | identisch | Referenz | Referenz | keine Divergenz |
| **EEPROM-Lesen** | ✔ gehärtet (Bridge + `get_string_descriptor`) | ✘ ungeprüft | ✘ = 433 | n/a | ✘ | AIS' Bounds-Check gehört in die gemeinsame Basis. |
| **RTL2832 Baseband/FIR** | identisch | identisch | identisch | Referenz | Referenz | keine Divergenz |
| **R820T/R820T2 PLL** | ✔ Fixpunkt (`vco_div`) | ○ iterative SDM-Schleife | ○ = 433 | ✔ Fixpunkt | ○ iterativ | osmocom/AIS sind exakter und schneller. |
| **R82xx I2C-Writes** | ✔ `shadow_equal` + Blockwrite | ✘ einzeln, ohne Elision | ✘ = 433 | ✔ | ✘ | Deutlich weniger I2C-Verkehr beim Tunen. |
| **R82xx VCO-Strom** | `0x80/0xe0` (osmocom) | `0x06/0xff` (Blog max) | = 433 | `0x80/0xe0` | `0x06/0xff` | Widersprüchliche Hardware-Einstellung — siehe 6.2. |
| **R82xx L-Band-Dropout** | ✘ (osmocom) | ✔ `div_buf_cur = 0xa0` | ✔ = 433 | ✘ | ✔ | Blog-Tweak, wirkt nur oberhalb ~1 GHz. |
| **V4 / V4L-Support** | ✔ | ✔ | ✔ | ✔ | ✔ | **In allen fünf vorhanden.** Kein Argument gegen osmocom. |
| **Android-Anpassungen** | ✔ | ✔ | ✔ | ✘ | ✘ | Gemeinsamer Ursprung (rtl_tcp_andro-Linie): libusb-Kompat-Makro, `get_string_descriptor`, `rtlsdr_close`-NULL-Guards, `libusb_interrupt_event_handler`. |
| **Fehlerbehandlung `rtlsdr_close`** | ✔ NULL-Guards | ✔ NULL-Guards | ✔ NULL-Guards | ✘ | ✘ | App-eigene Härtung, in beiden Upstreams nicht vorhanden. |
| **`convenience.c`** | ✔ gehärtet | ○ Upstream-Stand | ○ = 433 | ✘ Bug | ✘ Bug | AIS behebt einen echten OOB-Zugriff, den beide Upstreams haben. |
| **libusb `linux_usbfs.c`** | ✔ voll gepatcht | ✘ zwei Fixes fehlen | ✔ voll gepatcht | n/a | n/a | 433 ist hier die Schwachstelle. |

---

## 4. Herkunft und Pflegezustand

| | AIS | 433 | Pager |
| --- | --- | --- | --- |
| Remote | `ebc81/RTL_SDR_AIS_Driver_Andro` | `ebc81/rtlsdr433_android` | `ebc81/RTLSDR_Pager_android` |
| Erster Commit | 2026-04-29 | 2026-04-10 | 2026-08-20 |
| Commits an den Libs | **8** (bis 2026-08-23) | 3 (bis 2026-05-31) | **0** |
| rtl-sdr-Herkunft | osmocom-Linie, im Mai 2026 gegen blog `aed0ea1` abgeglichen | Blog-Fork-Snapshot, im April 2026 um V4L erweitert | 1:1 von 433 |
| libusb-Herkunft | eigene Pflege, Play-Console-getrieben | Blog-Zeit-Stand | von AIS + ein Merge-Fix aus 433 |

Alle drei sind squashed vendored imports — keine Submodules, keine Subtrees, keine
Upstream-Historie. Die AIS-Kopie ist älter als ihr Git-Repo (Play-Releases bis
versionCode 37 sind dort nicht dokumentiert).

`rtlsdrPager/AGENTS.md` dokumentiert die Herkunft bereits explizit und schreibt die Regel
*„take the union of fixes"* fest. Das ist die beste vorhandene Vorlage und sollte in die
gemeinsame Codebase übernommen werden.

**Es gab bereits einen Fix-Kreislauf:** 433 fand im Mai 2026 den Android-16-URB-Bug zuerst,
AIS übernahm ihn einen Tag später und trieb ihn weiter; Pager korrigierte im August die
Lock-Reihenfolge, AIS übernahm das drei Tage danach. Nur 433 hat aus dieser Runde nichts
zurückbekommen.

---

## 5. Differences between my three applications

### 5.1 Was identisch ist

- **`libusb-andro` bis auf eine Datei.** 27 von 28 Dateien byte-gleich, alle auf
  libusb 1.0.23 / `LIBUSB_NANO 11397`. Die Android-Anpassungen (Enumeration und
  Netlink-Hotplug per `#if defined(__ANDROID__)` deaktiviert, `fd_keep` in
  `libusb_wrap_sys_device`, `__android_log` als Log-Senke) sind überall gleich.
- **`rtl-sdr` zwischen 433 und Pager: vollständig.** Bis auf zwei `.bak`-Dateien in 433.
- **In allen drei Apps identisch:** `tuner_fc2580.c`, `reg_field.h`, `rtlsdr_i2c.h`,
  `rtl-sdr_export.h`, `convenience.h`, der komplette `getopt/`-Baum, alle Puffer- und
  Timeout-Konstanten (`DEFAULT_BUF_NUMBER 15`, `DEFAULT_BUF_LENGTH 16*32*512`,
  `CTRL_TIMEOUT 300`, `BULK_TIMEOUT 0`), die FIR-Koeffizienten, die Tuner-Tabelle.
- **Der Android-Öffnungsweg.** Alle drei machen dasselbe: `malloc` des privaten
  `rtlsdr_dev_t`, `libusb_init`, `libusb_wrap_sys_device(ctx, fd, &devh)`,
  `libusb_claim_interface`, Dummy-Register-Write mit `libusb_reset_device` als Fallback,
  `rtlsdr_init_baseband`, Tuner-Probe. `LIBUSB_OPTION_NO_DEVICE_DISCOVERY` wird nirgends
  benutzt — die Option gibt es erst ab libusb 1.0.24.
- **Der Resubmit-Check im Transfer-Callback** und die NULL-Guards in `rtlsdr_close`
  stehen wortgleich in allen drei Apps und in keinem der beiden Upstreams.

### 5.2 Was unterschiedlich ist, und welche Lösung besser ist

**a) R82xx-Registerzugriff — AIS deutlich besser**

AIS hat `shadow_equal()` (schreibt ein Register nicht, wenn der Wert schon anliegt) und
bündelt die Register 0x10–0x16 in einen einzigen I2C-Blockwrite. 433/Pager schreiben jedes
Register einzeln und ohne Elision. Beim Frequenzwechsel sind das in AIS eine Transaktion
statt sechs, plus alle unterdrückten Redundanzen. Auf USB-über-Android, wo jede
I2C-Transaktion ein Control-Transfer mit ~300 ms Timeout ist, ist das messbar.
**AIS gewinnt; das ist auch die osmocom-mainline-Lösung.**

**b) PLL-Berechnung — AIS besser**

AIS/osmocom: `vco_div = (pll_ref + 65536 * vco_freq) / (2 * pll_ref)`, dann
`nint = vco_div / 65536`, `sdm = vco_div % 65536`. Eine Division, exakt gerundet.
433/Pager/blog: iterative `while (vco_fra > 1)`-Schleife, die `sdm` bitweise aufbaut und
dabei Rundungsfehler akkumuliert. **AIS gewinnt** — gleiches Ergebnis im Normalfall,
aber ohne Drift an den Bereichsgrenzen und ohne Schleife.

**c) VGA-Gain bei Frequenzwechsel — 433/Pager haben hier eine Regression**

433/Pager rufen in `r82xx_set_freq()` bei **jedem** Tune `r82xx_set_vga_gain()` auf, das
Register 0x0c fest auf `0x08` (16,3 dB) setzt. Im AGC-Modus setzt `r82xx_set_gain()`
dieses Register aber auf `0x0b` (26,5 dB). Ergebnis: **bei aktiviertem AGC wird die
VGA-Verstärkung bei jedem Frequenzwechsel um rund 10 dB reduziert.** AIS und osmocom tun
das nicht. Für 433-MHz-ISM und POCSAG mit Auto-Gain ist das funktional relevant.
**AIS gewinnt.**

**d) Narrowing-Casts — 433/Pager besser**

433/Pager casten explizit in `rtlsdr_set_if_freq` (`(int32_t)`), `set_freq_correction`
(`(int16_t)`), `set_sample_rate` (`(uint32_t)`) und in `tuner_e4k.c` (2×). AIS und beide
Upstreams tun das nicht. Da alle diese Ausdrücke über `TWO_POW()` als `double` rechnen und
in Ganzzahlen zurückfallen, sind die Casts korrekt und dokumentieren die Absicht.
**433 gewinnt** — besonders, weil AIS `-Werror=shorten-64-to-32` auf genau die
Übersetzungseinheit anwendet, die `librtlsdr.c` inline einzieht.

**e) `!dev`-Guard in `rtlsdr_set_direct_sampling` — 433/Pager besser**

Beide Upstreams **und** AIS dereferenzieren `dev->direct_sampling_mode`, bevor auf NULL
geprüft wird. 433/Pager haben `if (!dev) return -1;` davorgesetzt. **433 gewinnt**,
klarer Null-Deref-Fix.

**f) `linux_usbfs.c` — AIS und Pager besser**

AIS und Pager nullen `urb->usercontext` vor `free(tpriv->urbs)` (an zwei Stellen) und
haben einen NULL-Guard in `reap_for_handle`. 433 hat nur den älteren
`if (!tpriv->urbs) return 0`-Guard, der voraussetzt, dass freigegebener Heap genullt ist —
was ohne MTE (Android < 16) nicht gilt. **AIS/Pager gewinnen.** Die beiden Dateien
unterscheiden sich nur noch im Kommentartext.

**g) `convenience.c` `set_gain_by_perc()` — AIS besser**

Bei Tunern ohne Gain-Tabelle liefert `rtlsdr_get_tuner_gains(dev, NULL)` 0 oder −1. Der
Upstream-Clamp `if (index >= (unsigned)count) index = (unsigned)(count - 1)` erzeugt dann
`0xFFFFFFFF` und indiziert eine Null-Byte-Allokation. AIS prüft `count <= 0`, klemmt
`percent` auf 100 und behandelt `malloc`-Fehler. **AIS gewinnt** — der Bug steckt in
beiden Upstreams.

**h) `get_string_descriptor()` — AIS besser**

AIS prüft `pos` gegen `EEPROM_SIZE` und klemmt das Längenbyte. 433/Pager (und beide
Upstreams) lesen `len = data[pos]` ungeprüft und laufen bei nicht gelesenem EEPROM bis zu
~260 Byte über den Stack-Puffer des Aufrufers hinaus. **AIS gewinnt.**

**i) Offset Tuning / Bias-T — hier gewinnen 433/Pager**

AIS behält den Blog-Hack, bei dem `rtlsdr_set_offset_tuning()` auf R820T/R828D den Bias-T
einschaltet, bevor es `-2` zurückgibt. Das legt bei einem Aufruf, der eigentlich „nicht
unterstützt" bedeutet, 5 V auf den Antennenanschluss. 433/Pager (und osmocom) geben
schlicht `-2` zurück. **433 gewinnt.**

### 5.3 Welche Version welche Zusatzänderungen hat

**Nur in AIS** — durchweg Portierungen von osmocom mainline plus eigene Härtung:
`shadow_equal()`, `mask_reg8()` + Blockwrite, Fixpunkt-PLL, `memset` der Shadow-Register,
`shadow_store()`-Fix `val -= r`, osmocom-VCO-Ströme, FC0012-Interpolationstabelle,
`get_string_descriptor()`-Bounds-Check, `set_gain_by_perc()`-Guards, aktives `force_bt`,
`rtlsdr_set_i2c_repeater(dev, 0)` im Direct-Sampling-Zweig, V4L-Meldung im R820T-Zweig,
Kernel-Driver-Detach, GPIO-Reset vor der FC2580/FC0012-Probe, EEPROM-Härtung im Bridge
(`memset` des Puffers, `eeprom_ok`, Tuner-Init-Fehler führt zu `goto err`), das ungenutzte
`rf_freq`-Feld.

**Nur in 433/Pager:** die fünf Narrowing-Casts, der `!dev`-Guard, die Blog-Hacks
(VCO-Strom max, L-Band-Dropout, VGA-Gain pro Tune), `r82xx_set_vga_gain()` als eigene
Funktion, `air_in`-Behandlung im V4L-Zweig, V4L-Meldung im R828D- statt R820T-Zweig,
FC0012-Default „Middle Gain" `0x08`, FC0013 `0x20`.

**Nur in Pager:** zwei zusätzliche Bridge-Funktionen `rtlsdr_last_open_was_busy()` und
`rtlsdr_is_dev_lost()`; der `__EBCANDROID__`-Kommentar in `linux_usbfs.c`. Am
rtl-sdr-Baum selbst: nichts.

**Nur in 433:** die beiden `.bak`-Dateien.

### 5.4 Was vermutlich versehentlich entstanden ist

1. **FC0013 Register 0x14 = `0x20` statt `0x40`** (`tuner_fc0013.c:255`, nur 433/Pager).
   Zeile 239 im selben `if`/`else`-Paar hat in allen fünf Varianten `0x40`; nur die
   zweite Stelle weicht ab. osmocom, blog und AIS haben dort `0x40`. Bit 6 statt Bit 5
   bedeutet eine falsche Bandumschaltung („enable UHF & disable GPS"). Kam mit dem
   Initial-Import von 433 herein, ist also ein Altbestand des verwendeten Snapshots und
   keine bewusste Änderung. **Ein-Zeilen-Korrektur.**

2. **V4L-Erkennungsmeldung im falschen Tuner-Zweig** (nur 433/Pager). osmocom, blog und
   AIS melden „RTL-SDR Blog V4 Lite Detected" im **R820T**-Zweig; 433/Pager haben sie in
   den R828D-Zweig verschoben. Rein diagnostisch — das tatsächliche Verhalten hängt an
   `rtlsdr_check_dongle_model()`, das an anderer Stelle aufgerufen wird. Aber die Meldung
   erscheint auf echter V4L-Hardware nicht.

3. **`force_bt` ist in 433/Pager toter Code.** Das Feld existiert, wird in
   `rtlsdr_set_bias_tee_gpio()` ausgewertet, aber nirgends gesetzt. Da `rtlsdr_open()`
   `memset(dev, 0, ...)` macht, ist es immer 0 — kein Bug, aber ein halb entfernter
   Feature-Rest.

4. **`rtlsdr_supporting_ppm_search()`** ist in allen drei `librtlsdr_andro.h` deklariert,
   aber nur in AIS definiert. In 433/Pager eine hängende Deklaration.

5. **Einrückungs-/Formatierungsartefakte** in beiden App-Varianten (Leerzeichen statt Tabs
   in `rtlsdr_open`, verrutschte Einrückung bei `dev->async_status = RTLSDR_CANCELING;` in
   AIS, entfernte Leerzeilen). Ohne Wirkung, erschwert aber jeden künftigen Upstream-Diff.

6. **`<android/log.h>` wird in allen drei `librtlsdr.c` inkludiert und nie benutzt.**
   Gleichzeitig geht die gesamte `fprintf(stderr, ...)`-Diagnose von rtl-sdr auf Android
   ins Leere — es gibt keine `stderr`-Umleitung. Alle Tuner-Erkennungs- und
   PLL-Fehlermeldungen der Bibliothek sind unsichtbar.

---

## 6. Upstream-Bewertung

### 6.1 Was den Blog-Fork von osmocom unterscheidet

rtl-sdr-blog V1.4.0 ist **kein** Rebase auf osmocom 2.0 — es ist ein eigenständiger Fork
mit eigener Historie. Der Unterschied ist trotzdem klein (85 Zeilen in `librtlsdr.c`,
171 in `tuner_r82xx.c`), weil beide V4/V4L-Support haben. Der Blog-Fork **fügt hinzu**:

- `enum rtlsdr_ds_mode` + `direct_sampling_mode` + `_rtlsdr_set_direct_sampling()`:
  Auto-Umschaltung auf Direct Sampling unterhalb 24 MHz bei R820T (V4L ausgenommen)
- `force_bt` aus EEPROM-Byte 7 und die Sperre in `rtlsdr_set_bias_tee_gpio()`
- Bias-T-Einschalten über `rtlsdr_set_offset_tuning()`
- `r82xx_toggle_test()` (Debug-Funktion)
- vier Hardware-Tweaks (siehe 6.2)

Und er **entfernt** gegenüber osmocom:

- `shadow_equal()`, `mask_reg8()`, den Blockwrite und die Fixpunkt-PLL
- `memset(priv->regs, 0, NUM_REGS)` beim Init und `val -= r` in `shadow_store()`
- die `dev_lost`-Prüfung in der Async-Schleife und `if (dev->dev_lost) return -1;`
  — stattdessen `rtlsdr_cancel_async()` direkt aus dem Transfer-Callback heraus, was
  `libusb_cancel_transfer()` auf allen Transfers aus einem laufenden Callback aufruft
- vier `rtlsdr_set_i2c_repeater(dev, 0)`-Aufrufe (auskommentiert)
- Init-Register 0x06 `0x32` → `0x30`, `filt_gain` `0x10` → `0x30`,
  `loop_through` `0x01` → `0x80`

Die Entfernungen sind aus meiner Sicht überwiegend Regressionen. Beide Apps haben die
`i2c_repeater`-Auskommentierung bereits rückgängig gemacht — das bestätigt die Einschätzung.

### 6.2 Die vier Blog-Hacks, einzeln bewertet

| Hack | Was er tut | Bewertung |
| --- | --- | --- |
| **VCO-Strom max** (`0x12`, `0x06/0xff` statt `0x80/0xe0`) | Setzt statt VCO-Strom „100" (Bits 7:5) ein anderes Bitmuster über die volle Maske und löscht damit die oberen Bits | Blogs Begründung ist bessere PLL-Lock-Stabilität am V4. Die Maske `0xff` überschreibt allerdings auch die `pw_sdm`-Bits, die osmocom gezielt setzt — die beiden Ansätze sind nicht kombinierbar. **Empfehlung: osmocom-Variante, Blog-Variante nur behalten, wenn ihr am V4 reproduzierbare Lock-Aussetzer messt.** |
| **L-Band-Dropout** (`div_buf_cur = 0xa0`) | Überschreibt den berechneten Wert hart, „PLL drop out 2.0 V" | Wirkt nur oberhalb ~1 GHz. Für AIS (162 MHz), 433 MHz und POCSAG (VHF/UHF) **irrelevant**. Kann entfallen. |
| **VGA-Gain pro Tune** (`r82xx_set_vga_gain()` in `set_freq`) | Setzt Register 0x0c bei jedem Frequenzwechsel auf 16,3 dB | **Schadet im AGC-Modus** (5.2c). Die Funktion ist im Blog-Original ohnehin bis auf eine Zeile auskommentiert. **Entfernen.** |
| **Bias-T über Offset-Tuning** | `rtlsdr_set_offset_tuning()` schaltet 5 V ein und gibt dann `-2` zurück | Workaround für fremde SDR-Software ohne Bias-T-Knopf. Eure Apps haben eigene UI. **Entfernen** — es ist eine überraschende Nebenwirkung auf einem Fehlerpfad. |

Bleibt als echter Blog-Mehrwert: **`force_bt` und der Auto-Direct-Sampling-Modus.** Beides
lässt sich problemlos auf osmocom 2.0.3 aufsetzen — AIS macht genau das bereits.

### 6.3 Migrationsbewertung

**Empfehlung: osmocom v2.0.3 als Basis, plus die drei Blog-Features, plus die
Android-Schicht.** Konkret:

Von osmocom v2.0.3 übernehmen (ist bereits AIS-Stand): kompletter `tuner_r82xx.c`
inklusive V4/V4L, `tuner_e4k.c`, `tuner_fc0012.c`, `tuner_fc0013.c`, `tuner_fc2580.c`,
`librtlsdr.c`-Grundgerüst mit `dev_lost`-Behandlung.

Aus dem Blog-Fork behalten: `enum rtlsdr_ds_mode` + Auto-Direct-Sampling,
`force_bt`-Auswertung, die öffentliche Deklaration von `rtlsdr_check_dongle_model()`.

Aus euren Apps behalten: die komplette Android-Schicht (fd-Open, Kompat-Makro,
`get_string_descriptor` gehärtet, `rtlsdr_close`-NULL-Guards, Resubmit-Check), die
Narrowing-Casts aus 433, den `!dev`-Guard aus 433, die `convenience.c`-Härtung aus AIS.

Verwerfen: L-Band-Dropout, VGA-Gain pro Tune, Bias-T-über-Offset-Tuning, `r82xx_toggle_test`,
`rf_freq` (ungenutzt), die `.bak`-Dateien.

Zu entscheiden: VCO-Strom (osmocom vs. Blog-max) — nur mit Messung an einem V4 zu klären.
*Inzwischen am V4 gemessen und entschieden: der osmocom-Wert bleibt — PROVENANCE.md §6.*

**Kosten:** Der Sprung ist für AIS minimal (es ist bereits dort). Für 433/Pager bedeutet er
einen Wechsel des R82xx-Verhaltens: andere VCO-Ströme, kein L-Band-Tweak, kein VGA-Reset
pro Tune. Das ist ein **echter Verhaltensunterschied an der Hardware** und muss auf einem
Gerät verifiziert werden, nicht nur kompiliert.

---

## 7. Befunde nach Priorität

| # | Befund | Betroffen | Schwere |
| --- | --- | --- | --- |
| 1 | `rtlsdr_open()` wertet den Rückgabewert von `rtlsdr_read_eeprom()` nicht aus; bei Lesefehler wird `force_bt` aus uninitialisiertem Stack abgeleitet und kann 5 V auf den Antennenanschluss legen | AIS (und blog upstream) | hoch |
| 2 | `get_string_descriptor()` liest ungeprüft über den Stack-Puffer hinaus | 433, Pager, beide Upstreams | hoch |
| 3 | `linux_usbfs.c` ohne `usercontext`-Nullen und ohne `reap_for_handle`-Guard → Use-after-free beim Abziehen auf Android 14/15 | 433 | hoch |
| 4 | `set_gain_by_perc()` indiziert bei Tunern ohne Gain-Tabelle eine Null-Byte-Allokation | 433, Pager, beide Upstreams | mittel |
| 5 | VGA-Gain wird im AGC-Modus bei jedem Tune um ~10 dB reduziert | 433, Pager | mittel |
| 6 | FC0013 Register 0x14 `0x20` statt `0x40` | 433, Pager | mittel (nur FC0013-Dongles) |
| 7 | `rtlsdr_cancel_async()` wird aus dem Transfer-Callback heraus aufgerufen | 433, Pager | mittel |
| 8 | `rtlsdr_set_direct_sampling()` dereferenziert `dev` vor der NULL-Prüfung | AIS, beide Upstreams | niedrig |
| 9 | Bias-T-Nebenwirkung in `rtlsdr_set_offset_tuning()` | AIS | niedrig |
| 10 | `fprintf(stderr, ...)` der Bibliothek geht auf Android verloren; `<android/log.h>` ungenutzt inkludiert | alle drei | niedrig (Diagnose) |
| 11 | `rtlsdr_supporting_ppm_search()` deklariert, nicht definiert | 433, Pager | niedrig |
| 12 | `.bak`-Dateien im Quellbaum | 433 | kosmetisch |

Befunde 1, 2 und 4 stecken auch in den Upstreams und sind Kandidaten für einen
Rück-Patch an osmocom.

---

## 8. Empfehlung für die gemeinsame Codebase

**Ausgangsbasis: der AIS-Baum**, nicht der von 433/Pager. Begründung: AIS entspricht
bereits osmocom v2.0.3 plus den erhaltenswerten Blog-Features, wird aktiv gepflegt (8
Commits gegen 3 bzw. 0) und ist gegen echte Play-Console-Crashes und auf Hardware
verifiziert worden.

**Rückzuportieren aus 433/Pager:**

1. die fünf Narrowing-Casts (`librtlsdr.c` 3×, `tuner_e4k.c` 2×) — nötig, damit AIS'
   `-Werror=shorten-64-to-32` weiter hält
2. `if (!dev) return -1;` in `rtlsdr_set_direct_sampling()`
3. die zwei Bridge-Funktionen des Pagers (`rtlsdr_last_open_was_busy`,
   `rtlsdr_is_dev_lost`) — nützlich für alle drei Apps
4. `linux_usbfs.c`: der Pager-Kommentar ist der genauere; inhaltlich ist AIS ≡ Pager

**Zusätzlich zu beheben, bevor der Baum geteilt wird:**

5. Befund 1 (EEPROM-Rückgabewert in `librtlsdr.c` — im Bridge ist es bereits gefixt)
6. Befund 9 (Bias-T-Nebenwirkung entfernen, osmocom-Verhalten herstellen)
7. Befund 6 (FC0013 `0x20` → `0x40`) — betrifft 433/Pager, muss beim Umstieg mitkommen
8. Befund 10 (`stderr` auf `__android_log_print` umleiten oder den toten Include entfernen)

**Nicht in die gemeinsame Basis:** `getopt/` (wird von keinem Projekt kompiliert),
`convenience.c` (nur AIS kompiliert es — als optionales Modul führen), die `.bak`-Dateien.

Die architektonischen Voraussetzungen (Inline-`#include` von `librtlsdr.c`,
zwei `rtlsdr_open2()`-Signaturen, divergierende Fehlercodes, `config.h`-Auflösung) sind in
`KONZEPT-GEMEINSAME-CODEBASE.md` beschrieben.

---

## Anhang: Reproduktion

Upstream-Clones neu anlegen:

```
git clone https://github.com/osmocom/rtl-sdr        libs_ebc\tmp\osmocom-rtl-sdr
git clone https://github.com/rtlsdrblog/rtl-sdr-blog libs_ebc\tmp\rtlsdrblog-rtl-sdr
git -C libs_ebc\tmp\osmocom-rtl-sdr    checkout v2.0.3
git -C libs_ebc\tmp\rtlsdrblog-rtl-sdr checkout V1.4.0
```

Jeder Befund dieses Berichts ist mit `diff -u` an den genannten Dateien nachvollziehbar,
zum Beispiel:

```
diff -u libs_ebc/tmp/osmocom-rtl-sdr/src/tuner_r82xx.c \
        RTL_SDR_AIS_Driver/app/src/main/jni/rtl-sdr/src/tuner_r82xx.c

diff -u rtlsdr433/app/src/main/cpp/rtl-sdr/src/tuner_fc0013.c \
        libs_ebc/tmp/osmocom-rtl-sdr/src/tuner_fc0013.c
```

Die drei App-Repos wurden während dieser Analyse nicht verändert; `git status --short`
war vor- und nachher identisch.
