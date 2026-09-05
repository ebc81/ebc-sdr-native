# Konzept: gemeinsame rtl-sdr-Codebase für alle EBC-Android-Apps

**Erstellt:** 2026-09-03
**Status:** umgesetzt (Stand 2026-09-05). Das Repo existiert als
`github.com/ebc81/ebc-sdr-native`, alle drei Apps pinnen Tag `v0.3.0` und sind auf Hardware
verifiziert; `rtlsdr433` ist als v1.3.3 ausgeliefert, `RTL_SDR_AIS_Driver` als v1.4.0 /
versionCode 56. **Phase 5 ist in allen drei App-Repos erledigt**; offen ist daran nur noch
eine Auslieferung, nicht Arbeit — Stand pro App in [AGENTS.md](AGENTS.md) *Legal posture*.
Der Text bleibt als Begründung stehen — er ist der Grund, warum es so gebaut wurde, nicht
bloß ein Plan.
**Grundlage:** [ANALYSE.md](ANALYSE.md)

---

## 1. Das Problem, kurz

Drei Apps, drei Kopien von `rtl-sdr` und `libusb-andro`. Der Analyse nach sind es faktisch
zwei Varianten (433 ≡ Pager), aber in beide Richtungen fehlen Fixes: AIS hat die besseren
Tuner- und EEPROM-Fixes, 433 die besseren Casts und den Null-Guard, 433 fehlen die
libusb-Unplug-Fixes.

Es gab bereits einen funktionierenden, aber manuellen Fix-Kreislauf: 433 → AIS (Mai),
Pager → AIS (August). Nur 433 hat aus dieser Runde nichts zurückbekommen. Genau diese
Asymmetrie soll die gemeinsame Codebase beseitigen.

---

## 2. Die GPL-Nebenbedingung

Das ist der Punkt, der die Architektur mitbestimmt — nicht nur eine Formalie.

`rtl-sdr` steht unter GPL-2.0-or-later, `libusb` unter LGPL-2.1-or-later. Jede App, die
diesen Code enthält und ausliefert, muss den entsprechenden Quelltext verfügbar machen.

### Was ohne Gegenmaßnahme passiert

Solange jede App ihre eigene Kopie von `rtl-sdr` und `libusb-andro` mitschleppt, liegt
derselbe GPL-Code mehrfach vor — und in unterschiedlichen Ständen. Dann gilt:

- Der veröffentlichte Quelltext jeder App zeigt auf einen anderen Stand desselben Codes.
- Ein Fix, der nur an einer Stelle landet, macht die anderen formal unvollständig.
- Der Aufwand pro Release vervielfacht sich statt zu sinken.

> Wie die einzelnen Apps ihren Quelltext verfügbar machen, entscheidet und dokumentiert jede
> App bei sich — im `NOTICE` der App und in ihrem eigenen Repository. Dieses Repo deckt den
> geteilten Teil ab, nicht mehr.

### Vorschlag

**Ein eigenes, von Anfang an öffentliches Repo für die geteilte Native-Basis.** Die
GPL-Pflicht ist damit strukturell erfüllt statt durch einen Kopiervorgang — es gibt keinen
Zustand mehr, in dem der veröffentlichte Quelltext hinter dem ausgelieferten Binary
zurückhängen kann.

---

## 3. Zielarchitektur

### 3.1 Repo-Layout

Neues öffentliches Repo, Arbeitstitel `ebc81/ebc-sdr-native`:

```
ebc-sdr-native/
  rtl-sdr/
    include/            rtl-sdr.h, tuner_*.h, reg_field.h, rtlsdr_i2c.h, rtl-sdr_export.h
    src/                librtlsdr.c, librtlsdr_internal.h, tuner_*.c
    src/convenience/    optional, per CMake-Option
  libusb-andro/
    libusb/             core.c, io.c, sync.c, descriptor.c, hotplug.c, strerror.c,
                        os/{linux_usbfs,poll_posix,threads_posix,linux_netlink}.c
    config.h            genau eine Datei
  android/
    librtlsdr_andro.c   fd -> rtlsdr_dev_t
    librtlsdr_andro.h
    ebc_log.h           aprintf_stderr / Log-Tag-Konfiguration
  CMakeLists.txt        add_library(ebc_sdr STATIC ...)
  PROVENANCE.md         Upstream + Tag + jeder lokale Patch mit Begründung
  CHANGELOG.md
  LICENSE / COPYING*
```

`libs_ebc\rtl-sdr` und `libs_ebc\libusb-andro` (heute leer) werden zum Arbeitsverzeichnis,
aus dem dieses Repo entsteht.

### 3.2 Einbindung in die Apps

Als **git submodule** unter `app/src/main/cpp/ebc-sdr-native`, im CMake:

```cmake
add_subdirectory(ebc-sdr-native)
target_link_libraries(<app-target> PRIVATE ebc_sdr ${log-lib})
```

Statt eines gemeinsamen `.so` bleibt es bei einem flachen Shared Object pro App — nur wird
die Bibliothek jetzt ein eigenes CMake-Target mit `target_include_directories(... PUBLIC)`
und `target_compile_options(... PRIVATE)`, statt Objektdateien in einem globalen Flag-Topf.

### 3.3 Warum Submodule

| | Submodule | Subtree | Kopie + Sync-Skript | Prebuilt AAR |
| --- | --- | --- | --- | --- |
| Version pro App pinnbar | **ja, sichtbar als Commit** | ja, aber unscheinbar | nein | ja |
| Stiller Drift möglich | nein | nein | **ja — der Ist-Zustand** | nein |
| GPL-Quelltextpflicht erfüllt | **ja, strukturell** | ja | nur bei Disziplin | **nein** — Binary ohne Quelle |
| Mirror-Sync entfällt für den geteilten Teil | **ja** | ja | nein | ja |
| Aufwand beim Klonen | `--recursive` nötig | keiner | keiner | keiner |
| Historie sauber getrennt | **ja** | vermischt sich | ja | ja |

Submodule gewinnt. Der einzige echte Nachteil ist `git clone --recursive` — verkraftbar,
und in `AGENTS.md`/`README` dokumentierbar.

Prebuilt-AAR scheidet aus: es löst die GPL-Quelltextpflicht nicht, sondern verschiebt sie
nur, und macht das Debuggen der Unplug-Pfade unnötig schwer.

### 3.4 Was mit den bestehenden Quelltext-Veröffentlichungen passiert

Der geteilte Teil — `rtl-sdr`, `libusb-andro` und die fd-Bridge — wird einmal hier
veröffentlicht, mit vollständiger Historie und eigenen Tags. Was app-spezifisch bleibt,
bleibt bei der jeweiligen App: bei rtlsdr433 der `rtl_433`-Baum, beim Pager `multimon/`,
jeweils samt der eigenen DSP- und JNI-Schicht. Fremder GPL-Code, den nur eine App
ausliefert, gehört nicht in dieses Repo.

Wer ein Sync-Skript für den app-spezifischen Teil betreibt, muss das Submodule-Verzeichnis
ausschließen — sonst wird derselbe Code an zweiter Stelle und in zweiter Version
veröffentlicht, also genau der Zustand, den dieses Repo beseitigt. Im Pager-Skript ist das
ein zusätzliches `/XD ebc-sdr-native` in den `robocopyArgs`.

> Die app-seitigen Details gehören zur jeweiligen App, nicht hierher.

**Empfehlung:** die drei Repos gleich taggen wie bisher, und im `ebc-sdr-native`-Repo
eigene Tags `v<major>.<minor>.<patch>` führen, die die Apps pinnen.

---

## 4. Technische Vorarbeiten

Diese Punkte müssen gelöst sein, bevor der Baum geteilt werden kann. Reihenfolge ist die
Abarbeitungsreihenfolge.

### 4.1 `librtlsdr.c` entkoppeln — der eigentliche Blocker

Heute ist `librtlsdr.c` in **keinem** Projekt eine CMake-Quelle. `librtlsdr_andro.c` zieht
sie per `#include "rtl-sdr/src/librtlsdr.c"` inline ein, weil der Bridge-Code an das
private `struct rtlsdr_dev` muss (er baut das Device-Struct selbst auf, statt
`rtlsdr_open()` zu benutzen). Das fixiert das relative Verzeichnis-Layout und verhindert
jede Auslagerung.

**Lösung:** ein interner Header `rtl-sdr/src/librtlsdr_internal.h` mit der
`struct rtlsdr_dev`-Definition und den bislang statischen Helfern, die der Bridge braucht
(`rtlsdr_init_baseband`, `rtlsdr_set_i2c_repeater`, `rtlsdr_write_reg`, die Tuner-Tabelle).
`librtlsdr.c` wird dann eine normale Übersetzungseinheit, `librtlsdr_andro.c` inkludiert
nur noch den Header. Das löst gleichzeitig die Duplicate-Symbol-Falle und den
Pfad-Constraint.

Aufwand: überschaubar, aber es ist ein echter Eingriff in eine Datei, die sonst nah am
Upstream bleiben soll. Der Header sollte deshalb *zusätzlich* angelegt werden, ohne
`librtlsdr.c` inhaltlich umzubauen.

### 4.2 `rtlsdr_open2()` vereinheitlichen

AIS: `rtlsdr_open2(rtlsdr_dev_t **out_dev, uint32_t index, int fd, const char *uspfs_path)`
— `index` und `uspfs_path` werden ignoriert (Altlast der Marinov-Signatur).
433/Pager: `rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd)`.

Auf die 2-stellige Form vereinheitlichen; in AIS den Aufruf in `rtl_ais_andro.c:1836`
anpassen.

### 4.3 Fehlercodes und Logging trennen

- `aprintf_stderr` ist in AIS eine **Funktion** aus `rtl_ais_andro.h`, in 433/Pager ein
  **Makro** auf `__android_log_print`. In der Bibliothek als Makro definieren, mit einem
  per `-DEBC_LOG_TAG="..."` konfigurierbaren Tag.
- Exit-Codes divergieren: AIS `-50/-51` (`rtlaisjava_err.h`), 433/Pager `-101/-102`.
  Die Bibliothek liefert einen eigenen Codebereich; das Mapping auf den app-eigenen
  bleibt in der App, damit der AIS-Java-Vertrag nicht bricht.
- Gleichzeitig Befund 10 aus der Analyse erledigen: entweder `stderr` der Bibliothek auf
  `__android_log_print` umleiten oder den toten `<android/log.h>`-Include entfernen.
  Umleiten ist die bessere Wahl — die Tuner-Erkennungs- und PLL-Fehlermeldungen sind beim
  Debuggen wertvoll.

### 4.4 `config.h` eindeutig machen

AIS löst `#include <config.h>` (aus `libusbi.h:26`) nach `libusb-andro/config.h` auf,
433/Pager nach `libusb-andro/libusb/config.h`. Die beiden unterscheiden sich nur in einer
Zeile, sind also semantisch gleich — aber es ist eine Falle. Zusätzlich liegt in 433 und
Pager ein **totes** `cpp/libusb_config.h` mit einem *anderen*, kleineren Definitionssatz
(ohne `ENABLE_LOGGING`, ohne `HAVE_LINUX_NETLINK_H`); wird das je versehentlich
eingebunden, ändert sich das libusb-Verhalten still.

Eine Datei behalten, die beiden anderen löschen.

### 4.5 Flags an das Target statt global

Heute stehen `-O1`, `-fvisibility=hidden`, `-fno-builtin-printf`, `-fno-builtin-fprintf`,
`-funwind-tables` in AIS' `CMAKE_C_FLAGS` bzw. Gradle `cFlags` und treffen damit auch
libusb und rtl-sdr; 433/Pager setzen nichts davon.

In der Bibliothek per `target_compile_options(ebc_sdr PRIVATE ...)` festlegen. Beachten:
`-fvisibility=hidden` ist unkritisch, weil `RTLSDR_API` in `rtl-sdr_export.h` explizit
`__attribute__((visibility("default")))` setzt.

### 4.6 16-KB-Page-Alignment für alle

`-Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384` steht heute **nur** in AIS'
CMakeLists. 433 und Pager verlassen sich implizit auf den NDK-r29-Default. Das ist
brüchig: ein NDK-Downgrade unter r28 bricht Android-15-Installationen auf 64-Bit-Geräten
bei grünem Build. Die Flags gehören in die gemeinsame CMake-Datei.

### 4.7 API-23-Kompatibilität wahren

AIS hat `minSdk 23`, 433 und Pager `29`. Die gemeinsame Native-Basis muss auf API 23
übersetzbar bleiben, sonst verliert AIS Geräte.

### 4.8 Kleinkram

- ~~`rtlsdr_supporting_ppm_search()` entweder überall definieren oder aus dem Header
  entfernen~~ — **erledigt.** Phase 1 hat sie zunächst überall definiert; in v0.3.0 ist sie
  ganz entfernt, weil sie nirgends einen Aufrufer hatte. Das PPM-Search-Subsystem, zu dem
  sie gehörte, wird im selben Zug aus AIS entfernt.
- `getopt/` und `convenience/` nicht in die Kernbibliothek: `getopt.c` wird von keinem
  Projekt kompiliert, `convenience.c` nur von AIS. Als optionales CMake-Target führen.
- Die beiden `.bak`-Dateien in 433 nicht übernehmen.
- Einheitliche `.gitattributes` mit `* -text` im neuen Repo — Pager hat das aus gutem
  Grund (LF-Garantie für vendorten Upstream-Code), und `core.autocrlf` ist unter Windows
  standardmäßig an.

---

## 5. Vorgeschlagene Reihenfolge

> **Stand:** Phasen 0 bis 4 sind abgearbeitet, in genau dieser Reihenfolge und ohne
> Abweichung. Was dabei herauskam, steht in [PROVENANCE.md](PROVENANCE.md) §3 bis §5 und in
> [CHANGELOG.md](CHANGELOG.md). Phase 5 ist in allen drei App-Repos erledigt; der Stand pro
> App steht an einer einzigen Stelle, in [AGENTS.md](AGENTS.md) *Legal posture*.

**Phase 0 — Union herstellen (in `libs_ebc\`, ohne die Apps anzufassen)** *(erledigt.)*
AIS-Baum als Basis nehmen, die vier Rückportierungen aus 433/Pager einarbeiten
(Narrowing-Casts, `!dev`-Guard, die zwei Pager-Bridge-Funktionen, der Pager-Kommentar in
`linux_usbfs.c`) und die Befunde 1, 6, 9, 10 aus der Analyse beheben. Ergebnis ist ein
Baum, der jede der drei heutigen Varianten dominiert. `PROVENANCE.md` dabei mitschreiben,
nicht danach.

**Phase 1 — Vorarbeiten 4.1 bis 4.8** im `libs_ebc`-Baum umsetzen und dort einmal
gegen ein Minimal-CMake bauen. *(erledigt.)*

**Phase 2 — Pilot: rtlsdrPager.** *(erledigt, Commit `2743190`.)* Grund: Pager hat *null*
eigene Änderungen an den Libs, minSdk 29, und die geringste Kopplung (kein `rtl_433`-Baum, der ebenfalls in rtl-sdr
hineinreicht). Wenn der Umbau dort trägt, trägt er überall. Auf echter Hardware
verifizieren, nicht nur bauen — Abschnitt 6.

**Phase 3 — rtlsdr433.** *(erledigt, Commit `7f7d7cb`, ausgeliefert als v1.3.3.)* Bekommt
dabei die fehlenden libusb-Unplug-Fixes, die FC0013- und VGA-Gain-Korrektur. Achtung: `rtl433/src/sdr.c` enthält einen `__EBCANDROID__`-Block, der
`rtlsdr_open2()` aufruft und eine eigene lokale Prototyp-Deklaration mitbringt — die muss
auf den Bibliotheks-Header umgestellt werden.

**Phase 4 — RTL_SDR_AIS_Driver.** *(erledigt, Commit `4ce3a9d`, ausgeliefert als v1.4.0 /
versionCode 56 am 2026-09-04.)* Zuletzt, weil dort die meisten Eigenheiten hängen:
`-Werror`-Vertrag auf drei Dateien, minSdk 23, eigener Fehlercode-Raum, Java- statt
Kotlin-Layer, und `librtlsdr_andro.c` inkludiert dort zusätzlich Projekt-Header
(`rtl_ais_andro.h`, `rtlaisjava_err.h`).

**Phase 5 — GPL-Umstellung.** *(erledigt.)* Dieses Repo öffentlich machen, die
app-seitigen Quelltext-Veröffentlichungen auf den app-spezifischen Teil reduzieren und in jeder
App auf das hier gepinnte Tag verweisen. Die app-seitigen Details gehören zur jeweiligen App.

> Dieses Repo ist öffentlich, und die app-seitige Hälfte ist in allen drei Repos geschrieben.
> Was daran noch offen ist, ist keine Arbeit mehr, sondern eine Auslieferung: die AIS-Texte
> gehen ohne eigenes Release mit dem nächsten regulären mit. **Der Stand pro App wird hier
> bewusst nicht wiederholt** — er steht in [AGENTS.md](AGENTS.md) *Legal posture* und wird
> dort gepflegt. Dieses Dokument bleibt der Plan und seine Begründung.

---

## 6. Wie zu verifizieren ist

Ein grüner Build beweist hier wenig — die Unterschiede sitzen im Hardwareverhalten.

- **Pro Phase auf echter Hardware:** Öffnen, Tunen über mehrere Bänder, Gain-Modus
  umschalten, Abziehen im laufenden Betrieb (das ist der Pfad, um den es bei den
  libusb-Fixes geht), erneut anstecken, Neustart der App ohne Neustart des Geräts.
- **Der VCO-Strom-Punkt (6.2 der Analyse) braucht eine Messung**, wenn ihr einen V4
  habt — sonst bleibt osmocom-Verhalten die Vorgabe.
- **Der VGA-Gain-Befund ist mit AGC direkt sichtbar:** Frequenz mehrfach wechseln und den
  Rauschteppich beobachten. Vorher (433/Pager) fällt er nach jedem Wechsel ab, nachher
  nicht mehr.
- **Alle vier ABIs bauen**, und bei AIS zusätzlich gegen API 23 prüfen.
- **`git status --short`** in den App-Repos vor und nach jedem Schritt vergleichen.

---

## 7. Offene Entscheidungen

1. ~~**Repo-Name** für die geteilte Basis.~~ **Entschieden:** `ebc81/ebc-sdr-native`.
2. ~~**VCO-Strom:** osmocom-Wert oder Blog-Maximum?~~ **Auf Hardware entschieden:** der
   osmocom-Wert bleibt. Am Blog V4 traten über zwei Bänder keine Lock-Aussetzer auf, also
   ist das Kriterium für den Blog-Hack nicht erfüllt — Messung in PROVENANCE.md §6.
   Messbar wurde das erst dadurch, dass Befund 10 die `PLL not locked`-Meldungen überhaupt
   nach logcat bringt.
3. ~~**Ob `libusb-andro` in dasselbe Repo kommt** oder in ein zweites.~~ **Entschieden:** ein
   Repo, getrennte Lizenzdateien pro Unterverzeichnis — siehe [LICENSE.md](LICENSE.md).
4. **Offen: ob der Kotlin-USB-Layer** (`UsbSession.kt`, `UsbOpenActivity.kt`, `NativeBridge.kt`)
   später ebenfalls geteilt wird. 433 und Pager haben dort heute stark divergierende
   Fassungen (Pager deutlich weiter), AIS hat einen Java-Layer. Das wäre ein eigenes
   Android-Library-Modul und ein eigenes Projekt — bewusst **nicht** Teil dieses Konzepts.
