---
package: timeSystem
summary: Library for precise absolute times in TT/TAI/UTC/TDB (leap seconds, MJD/JD/calendar formats), geocentric and barycentric arrival-time corrections from FT2 orbits and the JPL ephemeris, and the gtbary tool.
layer: io
depends_on: [tip, st_app, st_stream, st_facilities, facilities, hoops, cfitsio]
used_by: []
last_reviewed_commit: 4597d4f
---

# timeSystem Architecture

**timeSystem** represents moments of time precisely enough for pulsar
timing (sub-microsecond over decades), converts them between the TT, TAI,
UTC and TDB time systems, and applies geocentric and barycentric
corrections to Fermi LAT event files. It has three parts:

1. A **time library**: `AbsoluteTime`, `ElapsedTime`, `TimeInterval`,
   `Duration`, `TimeSystem` and the `TimeFormat` family (MJD, JD, calendar,
   ISO week, ordinal date).
2. **Arrival-time corrections**: `BaryTimeComputer` (JPL DE200/DE405), the
   `EventTimeHandler` abstraction over a FITS extension, its Fermi LAT
   implementations in `GlastTimeHandler.h`, and the **`gtbary`** tool.
3. **`PulsarTestApp`**, a unit-test harness written for the pulsar tool
   packages (`pulsarDb`, `pulsePhase`). Those packages are not built any
   more, so `test_timeSystem` is its only user.

The Doxygen mainpage (`src/mainpage.h`) is from 2009–2011. Two of its
claims are stale (see [Invariants and gotchas](#invariants-and-gotchas)):
`gtbary` no longer runs `axBary`, and the data files are not in `data/`.

## Purpose and scope

**Does:**
- Hold an absolute time as an integer MJD origin plus a `Duration`
  (whole days plus seconds), so that no precision is lost over long spans.
- Convert between TT, TAI, UTC and TDB. UTC uses a leap-second table read
  from a FITS file. TDB−TT comes from an analytic series (`ctatv.c`).
- Parse and format times as MJD, JD (each as integer + fraction or as one
  double), ISO 8601 calendar date, ISO week date and ordinal date.
- Compute geocentric (TT, `TIMEREF=GEOCENTRIC`) and barycentric (TDB,
  `TIMEREF=SOLARSYSTEM`) photon arrival times, from the spacecraft position
  in an FT2 file and the JPL DE200 or DE405 ephemeris. This includes the
  Roemer delay, the Earth-velocity term and the Shapiro delay, and parallax
  if a source distance is given.
- Rewrite every time column and time keyword of an FT1-like file
  (`gtbary`).

**Does not:**
- Handle pulsar ephemerides, phases or timing models. That was
  `pulsarDb`/`pulsePhase`, which are not built (see
  `docs/architecture/dependency-layers.md`, "Not built").
- Apply clock corrections. `gtbary` writes `TIMEZERO = 0`.
- Support the TCB or TCG time systems, though `bary.c` has functions for
  them.
- Provide the solar-system ephemeris used by `astro::SolarSystem`. `astro`
  has its own C++ copy of the same JPL reader (`src/astro/src/jplephem/`).
  The two copies share no code or state.
- Ship the reference data it needs (`JPLEPH.405`, `JPLEPH.200`,
  `leapsec.fits`). Those come from the `fermitools-data` conda package
  through `$TIMING_DIR`.

## Position in the system

| Direction | Package | Why |
|-----------|---------|-----|
| Depends on | `tip` | All C++ FITS access: event-file extensions in `EventTimeHandler`, the leap-second table, file copy and keyword updates in `gtbary`, and output comparison in `PulsarTestApp`. |
| Depends on | cfitsio | `PUBLIC`. Used directly by the C code: `glastscorbit.c` reads FT2 `START`/`SC_POSITION`, `dpleph.c` reads the JPL ephemeris FITS file. |
| Depends on | `st_app`, `hoops` | `TimeCorrectorApp` (gtbary) and `PulsarTestApp` derive from `st_app::StApp`. `hoops` is used directly for iterating parameters into HISTORY records. It arrives through `st_app`. |
| Depends on | `st_stream` | `operator<<` overloads for `st_stream::OStream` on every time class; `StreamFormatter` in `PulsarTestApp`. |
| Depends on | `st_facilities` | Only `Env::getEnv` and `Env::appendFileName`, to build the default leap-second path `$TIMING_DIR/leapsec.fits` (`TimeSystem.cxx`). |
| Depends on | `facilities` | `commonUtilities::joinPath` (gtbary) and `getDataPath("timeSystem")` (`PulsarTestApp`). Not in the CMake link list; it arrives transitively through `tip`/`st_app`. |
| Used by | — | Nothing in the Fermitools build includes timeSystem headers. `gtbary` is the only product used. |
| Used by (not built) | `skymaps` | `src/skymaps/src/PythonUtilities.cxx` uses `AbsoluteTime`, `Duration`, `ElapsedTime` and `BaryTimeComputer::getComputer("JPL DE405")`. skymaps is not in `FERMI_COMPONENTS`. |
| Used by (not built) | `pulsarDb`, `pulsePhase` | Historical users of `EventTimeHandler`, `GlastTimeHandler` and `PulsarTestApp`. Not submodules any more. |

The CMake link list (`cfitsio st_app st_stream st_facilities tip`) matches
actual use. `facilities` and `hoops` are used directly but are only linked
transitively.

**Tier.** timeSystem is in the `io` tier. All of its dependencies are in
tiers 0 and 1, and its library is an astronomy primitive with no
LAT-response knowledge. `gtbary` is the only tool it ships.

## Key abstractions

```mermaid
classDiagram
    direction LR
    class Duration {
      long day
      double sec  [0, 86400)
    }
    class TimeSystem {
      <<abstract, registry>>
      +getSystem(name)$
      +convertFrom(sys, moment)
      +computeDateTime(moment)
      +computeMoment(datetime)
    }
    TimeSystem <|-- TaiSystem
    TimeSystem <|-- TtSystem
    TimeSystem <|-- UtcSystem
    TimeSystem <|-- TdbSystem
    UtcSystem ..> LeapSecTable : singleton
    TdbSystem ..> ctatv : TDB-TT series
    class AbsoluteTime {
      TimeSystem* sys
      moment_type (origin MJD, Duration)
      +get(sys, rep) / set / represent
    }
    AbsoluteTime --> TimeSystem
    AbsoluteTime --> Duration
    class ElapsedTime { TimeSystem* sys; Duration }
    class TimeInterval { AbsoluteTime t1, t2 }
    AbsoluteTime ..> ElapsedTime : + / -
    AbsoluteTime ..> TimeInterval : t1 - t2
    class TimeFormat~Rep~ {
      <<abstract>>
      +convert(datetime) Rep
      +parse(string) Rep
      +format(rep, precision)
    }
    class BaryTimeComputer {
      <<abstract, registry>>
      +getComputer("JPL DE405")$
      +computeBaryTime(src, scpos, t)
      +computeGeoTime(src, scpos, t)
    }
    BaryTimeComputer <|-- JplDe200Computer
    BaryTimeComputer <|-- JplDe405Computer
    class EventTimeHandler {
      <<abstract>>
      +readTime / writeTime
      +getGeoTime / getBaryTime
      +initTimeCorrection(ft2, ext, eph, ...)
    }
    EventTimeHandler <|-- GlastTimeHandler
    GlastTimeHandler <|-- GlastScTimeHandler
    GlastTimeHandler <|-- GlastGeoTimeHandler
    GlastTimeHandler <|-- GlastBaryTimeHandler
    GlastScTimeHandler --> BaryTimeComputer
    GlastScTimeHandler ..> glastscorbit : FT2 position
```

### Time representation

| Type | Header | Role |
|------|--------|------|
| `Duration` | `Duration.h` | A length of time as `(long day, double sec)`, with `sec` always normalized to `[0, 86400)`. Construct with `Duration(value, "Day"|"Hour"|"Min"|"Sec")` or `Duration(int, frac, unit)`. Has no time system, so it is not a physical length on its own. |
| `moment_type` / `datetime_type` | `TimeSystem.h` | `moment_type = pair<long origin_mjd, Duration elapsed>`; `datetime_type = pair<long mjd, double sec_of_day>`. A moment is a time system's internal form. A datetime is the "MJD day plus seconds into that day" form that formats convert to and from. They differ only in UTC, where a day can have 86401 seconds. |
| `TimeSystem` | `TimeSystem.h` | Abstract registry of the four systems, looked up by case-insensitive name with `getSystem("TT")`. The concrete classes live in an anonymous namespace in `TimeSystem.cxx`. They do the conversions and the date arithmetic. Static functions manage the leap-second file (`loadLeapSeconds`, `setDefaultLeapSecFileName`). |
| `AbsoluteTime` | `AbsoluteTime.h` | A moment plus its time system. `get(sys, rep)`, `set(sys, rep)` and `represent(sys, fmt, precision)` convert systems and formats on the fly. Arithmetic: `+ ElapsedTime`, `- AbsoluteTime` gives `TimeInterval`, plus comparisons and `equivalentTo(other, tolerance)`. |
| `ElapsedTime` | `ElapsedTime.h` | A `Duration` measured in a particular time system, not tied to any moment. Used for "MET seconds in TT since MJDREF". |
| `TimeInterval` | `TimeInterval.h` | The interval between two `AbsoluteTime`s. It can be expressed in any system with `computeDuration(sys, unit)`, which is how MET is computed from an absolute time. |
| `TimeFormat<Rep>`, `TimeFormatFactory<Rep>` | `TimeFormat.h`, `MjdFormat.h`, `CalendarFormat.h` | Per-format singletons. Reps: `Mjd`, `Jd` (`m_int` + `m_frac`), `Mjd1`, `Jd1` (single double), `Calendar`, `IsoWeek`, `Ordinal`. Each header defines namespace-scope references `MjdFmt`, `Mjd1Fmt`, `JdFmt`, `Jd1Fmt`, `CalendarFmt`, `IsoWeekFmt`, `OrdinalFmt`. |
| `IntFracUtility` | `IntFracUtility.h` | Parses and formats "integer.fraction" strings without losing digits. Used by the MJD/JD formats. |
| `TimeConstant.h` | | `SecPerDay()` etc., returned as `long`. |

Conversion graph, from `TimeSystem.cxx`:

```
UTC --(TAI-UTC = 10 s + cumulative leap seconds)--> TAI --(+32.184 s)--> TT --(+ ctatv(JD_TT))--> TDB
```

Every other pair goes through these steps. TDB→TT is iterative: up to
100 iterations, converging to 100 ns, the accuracy of `ctatv`.

### Arrival-time corrections

| Type | Header | Role |
|------|--------|------|
| `SourcePosition` | `SourcePosition.h` | A unit direction from RA/Dec (degrees) or from a 3-vector, with an optional distance in light-seconds. If a distance is given, `JplComputer` adds parallax (curved-wavefront) terms. |
| `BaryTimeComputer` | `BaryTimeComputer.h` | Registry keyed by ephemeris name (`"JPL DE200"`, `"JPL DE405"`). The first `getComputer` call initializes the ephemeris through `initephem()` in `dpleph.c`. `computeBaryTime` adds the delay as an `ElapsedTime` in **TDB**. `computeGeoTime` adds it in **TT**. |
| `EventTimeHandler`, `IEventTimeHandlerFactory`, `EventTimeHandlerFactory<T>` | `EventTimeHandler.h` | Wraps one extension of a FITS file and walks its rows. Subclasses decide whether they support a file (`static createInstance` returns 0 if not), and read and write times as `AbsoluteTime`. The factory registry was for pulsar tools that accept several missions. `gtbary` does not use it. |
| `GlastScTimeHandler` | `GlastTimeHandler.h` | Accepts `TELESCOP` = `GLAST` or `FERMI`, `INSTRUME=LAT`, `TIMEREF=LOCAL` (default if absent) and `TIMESYS=TT` (default). Opens the FT2 file via `glastscorbit_open` and computes corrected times. |
| `GlastGeoTimeHandler` | `GlastTimeHandler.h` | `TIMEREF=GEOCENTRIC`, `TIMESYS=TT`. Already geocentered. `getBaryTime` throws. |
| `GlastBaryTimeHandler` | `GlastTimeHandler.h` | `TIMEREF=SOLARSYSTEM`, `TIMESYS=TDB`. Already barycentered. Checks `RA_NOM`/`DEC_NOM` against the requested position (within `angtol`) and `PLEPHEM` against the requested ephemeris. `getGeoTime` throws. |
| `glastscorbit_*` (C) | `glastscorbit.h` | Opens and caches FT2 files and interpolates `SC_POSITION` at a MET. Interpolation is linear in radius and in orbital phase (angle in the plane of the two neighbouring rows), not in Cartesian coordinates. |
| `TimeCorrectorApp` | `TimeCorrectorApp.h` | The `gtbary` application. |

MET convention (`GlastTimeHandler`): the time system is the header's
`TIMESYS` (TT if absent). `MJDREFI`+`MJDREFF`, or a string `MJDREF`, is
read from the header. The default is MJD 51910 + 64.184 s, so MET 0 is
2001-01-01T00:00:00 UTC expressed in TT.

### C code: what is used and what is not

The C sources are Arnold Rots' HEASARC barycentering code (`axBary`,
`bary.c`) plus Fermi additions. **All of them are compiled** into
`libtimeSystem.a`, but the C++ code calls only some of them:

| File | Lines | Used by the C++ code? |
|------|------:|-----------------------|
| `dpleph.c` | 665 | Yes. `initephem`, `dpleph` (`BaryTimeComputer.cxx`). |
| `ctatv.c` | 206 | Yes. TDB−TT (`TimeSystem.cxx`). |
| `glastscorbit.c` | 519 | Yes. FT2 positions (`GlastTimeHandler.cxx`). |
| `bary.c` | 1398 | Only `openFFile`, which `dpleph.c` uses to find `JPLEPH.*` in `$TIMING_DIR`. The rest (its own UTC/TT/TDB conversions from `tai-utc.dat`, `baryinit`, `barycorr`, pulsar phase) is unused. |
| `axBary.c` | 712 | **No.** The original `axBary` driver that the mainpage says `gtbary` is based on. `gtbary` reimplements it in C++. |
| `phaseHist.c` | 1636 | **No.** |
| `scorbit.c`, `clock.c`, `xCC.c` | 639 | **No.** XTE, AXAF and Swift orbit readers and clock corrections. |

About 3,000 of the roughly 6,000 lines of C are never called. Because the
library is static, the unused objects are not linked into `gtbary`.
Hand-written C++: about 4,600 lines in `src/*.cxx` plus 2,200 in headers.
The test program is another 4,900.

## Data flow and control flow

### `gtbary` (`TimeCorrectorApp::run`)

```
Prompt + Save all parameters
tcorrect=BARY → target TIMESYS=TDB, TIMEREF=SOLARSYSTEM, handler pair (GlastSc → GlastBary)
tcorrect=GEO  → target TIMESYS=TT,  TIMEREF=GEOCENTRIC,  handler pair (GlastSc → GlastGeo)
Check evfile exists; FOR EVERY HDU (primary included) try GlastScTimeHandler::createInstance
    → any HDU that is not a LAT/TT/LOCAL extension ⇒ error "Unsupported timing extension"
clobber check; open outfile for append (creates it if missing) to test writability
tip copyFile(evfile → outfile + ".tmp")
solareph → PLEPHEM ("JPL-DE200"/FK5 or "JPL-DE405"/ICRS)
TimeSystem::setDefaultLeapSecFileName(leapsecfile)        // "DEFAULT" ⇒ $TIMING_DIR/leapsec.fits
For each HDU of the tmp file: set TIMESYS, TIMEREF, RA_NOM, DEC_NOM, RADECSYS, PLEPHEM,
    TIMEZERO=0, CREATOR, DATE (UTC), TIERRELA (kept, else 1e-9); FILENAME if present
For each HDU:
    input = GlastScTimeHandler(evfile), output = GlastBary/GeoTimeHandler(tmp, writable)
    add HISTORY: every parameter
    input.initTimeCorrection(scfile, sctable, solareph)    // glastscorbit_open + BaryTimeComputer::getComputer
    input.setSourcePosition(ra, dec)
    keywords TSTART, TSTOP, DATE-OBS, DATE-END (if present) → corrected → output
    rows: GTI ⇒ START, STOP columns; any other table ⇒ `timefield` column
        MET → AbsoluteTime(TT) ; glastscorbit_calcpos(MET) ; JplComputer delay ; write MET in output TIMESYS
remove(outfile); rename(tmp → outfile)
```

The barycentric delay (`JplComputer::computeTimeDelay`):
`(r_obs/c + r_EarthSSB)·n̂ + (r_obs·v_Earth)/c² + 2·GM_sun/c³·ln(1 + cosθ)`.
The Earth position and velocity come from `dpleph(JD_TT, Earth, SSB)`.
`r_obs` is the FT2 `SC_POSITION` in metres. The geocentric delay is only
`(r_obs/c)·n̂`.

### Time conversions in a typical library call

```cpp
using namespace timeSystem;
// MET 3e8 s in TT → TDB MJD string
AbsoluteTime t = AbsoluteTime("TT", 51910, 64.184) + ElapsedTime("TT", Duration(3.e8, "Sec"));
std::string s = t.represent("TDB", MjdFmt);          // "5xxxx.xxxxxxxx (TDB)"
// Back to MET:
double met = (t - AbsoluteTime("TT", 51910, 64.184)).computeDuration("TT", "Sec");
```

Do not convert MET through a single `double` MJD. That loses about 1 µs.
Keep the `(long, double)` forms (`Mjd`, `Duration`) and do arithmetic with
`ElapsedTime`/`TimeInterval`.

## Public interfaces

### Applications

| Tool | Par file | Notes |
|------|----------|-------|
| `gtbary` | `pfiles/gtbary.par` | Queried: `evfile`, `scfile`, `outfile`, `ra`, `dec`. Hidden: `tcorrect` (BARY\|GEO), `solareph` (`JPL DE200`\|`JPL DE405`), `angtol` (deg), `timefield` (`TIME`), `sctable` (`SC_DATA`), `leapsecfile` (`DEFAULT`), plus the standard `chatter`/`clobber`/`debug`/`gui`/`mode` (see `src/st_app/doc/architecture/`). `outfile` may equal `evfile` for in-place correction. |

`test_timeSystem` is also **installed** to `bin/` (it is in the `install(TARGETS …)` list).

User help text lives in `src/fermitools-fhelp/fhelp_files/gtbary.txt`.

### C++ library

All headers in `timeSystem/` are installed. The ones a client needs:
`AbsoluteTime.h`, `ElapsedTime.h`, `Duration.h`, `TimeInterval.h`,
`TimeSystem.h`, `MjdFormat.h`, `CalendarFormat.h`, `SourcePosition.h`,
`BaryTimeComputer.h`, `EventTimeHandler.h`, `GlastTimeHandler.h`. The
`glastscorbit.h` C API is also installed. `bary.h` is private
(`src/bary.h`).

`PulsarTestApp.h` (installed, compiled into the library) provides
`PulsarTestApp` and `PulsarApplicationTester`. They find test inputs with
`getDataPath(<package>)` and reference outputs in `<data>/outref/`, and
compare FITS and text outputs card by card and cell by cell. Its include
guard still says `pulsarDb_PulsarTestApp_h`.

### Python

None.

### Environment variables

| Variable | Read by | Effect |
|----------|---------|--------|
| `TIMING_DIR` | `bary.c` `openFFile` (via `dpleph.c` `initephem`/`readephem`) | Directory holding `JPLEPH.405` / `JPLEPH.200`. If the file is not found there, `$LHEA_DATA` is tried. |
| `TIMING_DIR` | `TimeSystem::getDefaultLeapSecFileName` (via `st_facilities::Env::getEnv`) | Default leap-second table `$TIMING_DIR/leapsec.fits`. **Throws** if `TIMING_DIR` is unset and no explicit file is given. |
| `LHEA_DATA` | `bary.c` `openFFile` | Fallback directory for the JPL files (HEASoft convention). |
| `FERMI_INST_DIR` (via `facilities`) | `PulsarTestApp` `getDataPath("timeSystem")` | Test data and `outref/`. |

Conda activation sets `TIMING_DIR` to
`$CONDA_PREFIX/share/fermitools/refdata/fermi/jplephem` (see "Runtime
environment" in `docs/architecture/README.md`). That directory comes from
the `fermitools-data` conda package (pinned `=0.19` in `recipe/meta.yaml`),
not from this repository. It holds `JPLEPH.200`, `JPLEPH.405`,
`leapsec.fits` and `tai-utc.dat`. Only the first three are read by code
that runs.

### File formats

- **Event input:** any FITS file whose **every** HDU (primary included)
  has `TELESCOP` = `GLAST`/`FERMI`, `INSTRUME=LAT`, and
  `TIMEREF`/`TIMESYS` = `LOCAL`/`TT` (or absent). Time columns are
  `timefield` in non-GTI tables and `START`/`STOP` in `GTI`. Keywords
  `TSTART`, `TSTOP` (MET) and `DATE-OBS`, `DATE-END` (UTC ISO 8601 with
  `T`) are converted.
- **Spacecraft input (FT2):** extension `sctable` with the same header
  checks, a `START` column (time-ordered) and `SC_POSITION` (metres,
  inertial).
- **Output:** a copy of the input with times replaced and keywords
  `TIMESYS`, `TIMEREF`, `RA_NOM`, `DEC_NOM`, `RADECSYS`, `PLEPHEM`,
  `TIMEZERO`, `CREATOR`, `DATE`, `TIERRELA` and HISTORY added or updated in
  every HDU.
- **Leap-second table:** OGIP `leapsec.fits`. Extension 1 must have `MJD`
  (integral) and `LEAPSECS` (integral) columns.
- **JPL ephemeris:** HEASARC FITS conversion of DE200/DE405. Constants are
  in an extension, coefficients are in HDU 4.

## Build and test

- **CMake targets:** `timeSystem` (**STATIC**, PIC via the global
  setting), `gtbary`, `test_timeSystem`.
- **Install:**
  - headers go to `include/timeSystem`
  - `pfiles/gtbary.par` goes to `syspfiles`
  - `data/` (test inputs, including `outref/`) goes to
    `${FERMI_INSTALL_REFDATADIR}/timeSystem`
  - `data/outref/` also goes to `${FERMI_INSTALL_DATADIR}/timeSystem/outref`.
    Nothing reads that second copy: `PulsarTestApp` looks under
    `getDataPath("timeSystem")`, which is the refdata copy.
- **`test_timeSystem`** (`src/test/test_timeSystem.cxx`):
  - It is an `st_app` program run through `PulsarTestApp::run`. It is
    **not registered with CTest**. It has no par file of its own, which
    `st_app` tolerates, but its `gtbary` test builds an
    `AppParGroup("gtbary")`, so `PFILES` must find `gtbary.par`.
  - Needs `FERMI_INST_DIR` (data via `getDataPath`) and `TIMING_DIR`
    (`JPLEPH.405` for the `BaryTimeComputer`, `GlastTimeHandler` and
    `gtbary` tests, and `leapsec.fits` for the `gtbary` test, which uses
    `leapsecfile=DEFAULT`).
  - The unit tests use their own leap-second tables, `data/testls.fits`
    and `data/bogusls.fits`, so their UTC results do not depend on the
    installed table.
  - Test groups, all run: `Duration`, `TimeSystem`, `AbsoluteTime`,
    `ElapsedTime`, `TimeInterval`, `TimeFormat`, `IntFracUtility`,
    `SourcePosition`, `BaryTimeComputer`, `EventTimeHandlerFactory`,
    `glastscorbit`, `GlastTimeHandler`, `TimeCorrectorApp`. Two
    `#if 0` blocks in `testTimeSystem` are obsolete UTC-origin tests
    (disabled in 2008).
  - The `gtbary` test (`testTimeCorrectorApp`) runs BARY (`par1`) and GEO
    (`par2`) corrections on `testevdata_1day_unordered.fits` and compares
    the output with `data/outref/testTimeCorrectorApp_par{1,2}.fits`. The
    other cases compare logged error text against `.ref` files that the
    test writes itself.
  - **Scratch output:** test logs and FITS files go to the **current
    directory**. `testGlastTimeHandler` also writes
    `testevdata_1day_copy.fits` **into the data directory**, so the test
    needs write access to `$FERMI_INST_DIR/refdata/fermi/timeSystem`.
  - On failure it throws at the end of `run()`, after printing every
    mismatch.
- **Legacy:** `SConscript` (old SCons build) and `azure-pipelines.yml`.

## Invariants and gotchas

**Expiring and stale reference data**
- **The leap-second table in use is out of date.** The `leapsec.fits` in
  `fermitools-data` 0.19 (the version pinned in `recipe/meta.yaml`) has
  26 rows. Its last leap second is **2012-07-01**. It is missing
  **2015-07-01** and **2017-01-01**. `LeapSecTable::getCumulativeLeapSec`
  returns the last entry for any later date and gives **no warning**. So
  every UTC↔TT/TAI/TDB conversion for dates after 2015-07-01 is off by
  1 s, and after 2017-01-01 by **2 s** (checked by reading the FITS file;
  impact from reading the code).
  - In `gtbary` the error appears only through `DATE-OBS`/`DATE-END`. They
    are read and written as UTC with the same table, so the offsets almost
    cancel. MET columns and `TSTART`/`TSTOP` never pass through UTC.
  - Any other client that converts MET to UTC (for example `skymaps`, or
    a script using the library) is off by 2 s.
  - `tai-utc.dat` in the same directory also stops at 2012, but only the
    unused `bary.c` code reads it.
- **JPL DE405** (`JPLEPH.405`) covers JD 2433264.5–2469808.5, that is
  **1949-12-14 to 2050-01-02**. After that, `dpleph` returns NULL and
  `gtbary` throws "Could not find solar system ephemeris".
- There is no TDB−TT table. `ctatv.c` is an analytic series, accurate to
  100 ns.

**Process-wide state**
- Only **one JPL ephemeris per process**. After `getComputer("JPL DE405")`
  has initialized, asking for DE200 throws "cannot coexist". The C reader
  (`dpleph.c`) keeps the open file, the record buffer and the constants in
  file-scope statics, and reopens the FITS file each time the requested
  time moves into a new record.
- The leap-second table is a lazily loaded singleton. `getSystem("UTC")`
  loads it with `force_load=false`, so a later
  `setDefaultLeapSecFileName` has no effect unless you call
  `TimeSystem::loadLeapSeconds(name, true)`. `gtbary` sets the default
  before the first UTC conversion, so it gets the file it asked for.
- `glastscorbit_open` caches open FT2 files by (file name, extension
  name) in a static table.

**gtbary behaviour**
- **Every HDU must look like LAT data in TT/LOCAL.** A file with an extra
  HDU that lacks `TELESCOP`/`INSTRUME` (or has another instrument) is
  rejected. A file that has already been barycentered (TDB/SOLARSYSTEM) is
  also rejected, because the input side always uses `GlastScTimeHandler`.
- **Temporary file and failures.** The output is built in
  `<outfile>.tmp` and renamed at the end. On error, `run()` has no cleanup
  path, so the `.tmp` file is left behind whether or not `debug=yes`.
  The mainpage says it is kept only in debug mode, which is not what the
  code does (from reading the code; not checked at runtime). The
  writability check opens `outfile` in append mode, which creates an
  **empty `outfile`** if it did not exist. That empty file is also left
  behind on a later error. The return values of `std::remove` and
  `std::rename` are not checked.
- `DATE-OBS`/`DATE-END` must have the `T` separator
  (`YYYY-MM-DDThh:mm:ss`). A date-only value throws "Missing separator"
  (`CalendarFormat.cxx`).
- `TIMEZERO` is set to 0 and `TIERRELA` defaults to 1e-9, following
  `axBary.c`.
- The FT2 `START` column is searched with `bsearch`, so it must be in
  time order. Nothing checks this (from reading the code). There is a
  1 ms tolerance at the first and last rows. Outside that, the result is
  "the time is not covered by spacecraft file".
- `GlastScTimeHandler::initTimeCorrection` ignores `angtol` and
  `match_solar_eph`. They matter only when the input is already
  barycentered (`GlastBaryTimeHandler`), which `gtbary` never does.

**Library semantics**
- `ElapsedTime` belongs to a time system. Adding `ElapsedTime("TT", 1 s)`
  to an `AbsoluteTime` held in UTC first converts to TT. Across a leap
  second, "1 s in TT" and "1 s in UTC" give different calendar results.
- UTC is undefined before the first table entry (1972-01-01). Asking for
  an earlier UTC time throws.
- `AbsoluteTime::write` has a duplicated `if (datetime.second > 0.)`
  condition, so its "seconds before" branch is unreachable. That is
  harmless, because the seconds are always non-negative.
- `MjdFmt`, `CalendarFmt` and the others are namespace-scope `static`
  references defined in headers, so every translation unit has its own
  copy. They bind to function-local singletons, so static-initialization
  order is safe.

**Stale docs**
- `src/mainpage.h` says `gtbary` is "taken from axBary" and that the data
  files "are included in the data subdirectory". Neither is true now (see
  the C-code table and Environment variables). The help file
  `src/fermitools-fhelp/fhelp_files/gtbary.txt` repeats the second claim.

**Unmerged branches** (as of `4597d4f`):
- `origin/f26-cpp17-compliance` (2026-10-16): C++17 cleanups across 15
  files, including `TimeSystem.cxx` and the test.
- `origin/addition_creator_fits` (2025-08): sets `CREATOR` from
  `GenericUtils::creator_banner` in `TimeCorrectorApp`.
- `origin/Issue17` (2022): optional static libraries.

## Extension points

- **New time system** (for example TCB): add a `TimeSystem` subclass in
  the anonymous namespace of `TimeSystem.cxx`, add a static instance in
  `TimeSystem::getSystem`, and add conversion branches both in the new
  class's `convertFrom` and in each existing class's `convertFrom`. The
  conversion graph is hard-coded, not table-driven.
- **New time format:** define a rep struct, a `TimeFormat<Rep>`
  implementation, a `TimeFormatFactory<Rep>` specialization, and a
  `XxxFmt` reference, following `MjdFormat.h`.
- **New planetary ephemeris:** subclass `JplComputer` (or
  `BaryTimeComputer`) in `BaryTimeComputer.cxx`, add a static instance in
  `getComputer`, add the option to `gtbary.par`'s `solareph` and to the
  `PLEPHEM` mapping in `TimeCorrectorApp::run`. Put the file in
  `$TIMING_DIR`.
- **New mission or event-file type:** subclass `EventTimeHandler`
  (see `GlastTimeHandler`), and provide a static `createInstance` that
  returns 0 for unsupported headers. `gtbary` hard-codes its handler pairs
  in `TimeCorrectorApp::run`, so it needs a new `HandlerPairFactory`
  there.
- **Updating leap seconds:** replace `leapsec.fits` in the
  `fermitools-data` package (the HEASARC CALDB copy is current). No code
  change is needed. To override per run, use `gtbary leapsecfile=<file>`.

Before changing conversion code, run `test_timeSystem` with
`FERMI_INST_DIR`, `PFILES` and `TIMING_DIR` set, from a scratch directory.

## Further reading

- `src/mainpage.h`: the original overview and parameter descriptions
  (partly stale, see above).
- `doc/release.notes`: history to 2011.
- `src/bary.h`, `src/axBary.c`: Arnold Rots' original documentation of
  the barycentering algorithm and file search.
- `src/glastscorbit.c`: the comment on the 1 ms FT2-boundary tolerance.
- `src/astro/doc/architecture/`: the separate JPL reader used by
  `astro::SolarSystem`.
- `src/skymaps/doc/architecture/`: the unbuilt consumer.
- OGIP leap-second file format; HEASoft `barycorr` (the HEASARC
  counterpart to `gtbary`).
