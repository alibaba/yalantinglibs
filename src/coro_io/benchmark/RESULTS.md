# Own-ring benchmark results — 2026-09-14

Tested against public upstream `alibaba/yalantinglibs` commit
`27b04dd754fa1d9f3ae97dcf48a11ded160f7bf5`, plus this change.
The native baseline is vendored **Asio 1.24.0** (`ASIO_VERSION=102400`), not
another build of the own-ring implementation. No Asio sources were modified.

## Configuration

- Linux 6.6-series kernel, GCC 12.3.1 Release build, liburing 2.3, fio 3.34.
- Intel Xeon 6967P-C, ext4 on storage backed by two NVMe devices.
- Shared host and non-exclusive CPU affinity; these are short synthetic tests,
  not application throughput or device utilization measurements.
- `YLT_ENABLE_FILE_IO_URING=ON`, `YLT_ENABLE_OWN_RING=OFF`. Both explicitly
  selected backends run in the same executable. CSV records the Asio version
  and default-backend switch for reproducibility.

## Same-file Asio comparison

64 MiB patterned, non-sparse file; 4 KiB O_DIRECT reads; CPUs `327,349`;
30,000 measured reads per run; three repetitions with rotated backend order.
Every read verifies the error, length, and complete buffer contents. Each table
entry is the median of that metric, not a selected best run.

| Same-file QD | Native IOPS | Own IOPS | Ratio | Native P99 µs | Own P99 µs |
|---:|---:|---:|---:|---:|---:|
| 1 | 8,914 | 8,321 | 0.93× | 300.6 | 235.6 |
| 32 | 7,237 | 244,243 | 33.75× | 25048.3 | 270.8 |
| 128 | 6,706 | 347,895 | 51.88× | 85281.8 | 514.5 |

The native path stays approximately serialized for this shared file object;
additional readers increase waiting rather than throughput. Own-ring allows
independent reads to remain in flight. **QD1 is slightly slower** in this run;
the change is not a universal single-request latency/throughput improvement.

### Independent-file-object control

With 32 independent file objects referring to the same data file and total QD32,
native reaches 180,788 IOPS and own-ring 200,015 IOPS: **1.11×**, not tens of
times faster. This control supports removing per-object serialization as the
main source of the large same-object gains. Applications already exposing
file-level parallelism should not assume the same speedup.

All 24 Asio/own runs completed 720,000 measured reads with zero errors, short
reads, or content mismatches. Raw CSVs are in `results/native-vs-own.csv` and
`results/many-files.csv`.

## Matched 16-owner / 16-job fio comparison

One shared 1 GiB file; 4 KiB O_DIRECT; total QD2048 (128 per owner/job); one-second
ramp, five-second measurement, three repetitions with rotated profile order.
Both tools are pinned one-to-one to the same 16 distinct physical cores:
`41,43,45,60,195,196,201,206,212,219,240,250,257,273,275,276`.
The earlier two-CPU Asio experiment uses different CPU/NUMA placement and is
not a strict scaling baseline for this table.

| Profile | Median IOPS | P99 µs | CPU µs/IO | Relative to fio |
|---|---:|---:|---:|---:|
| Own, benchmark content comparison disabled | 4,011,653 | 1010.6 | 3.406 | **90.9%** |
| fio io_uring, batched submission/completion | 4,413,341 | 725.0 | 2.994 | 100% |
| Own, complete content comparison enabled | 3,560,298 | 1026.5 | 3.928 | 80.7% |

The no-content-comparison profile still validates errors and lengths. It does
not turn off safety checks in the production API. The checked profile verifies
52,888,485 measured reads with zero errors. fio uses ordinary fd/buffers with
no registered files, fixed buffers, SQPOLL, or IOPOLL in this comparison.
Per-run metrics are in `results/fio-16.json`; hostnames, private paths, and
machine-specific command transcripts are not included in the public artifacts.

The ratio is **throughput relative to the measured fio configuration**, not a
percentage of absolute hardware capability. The small working set does not
exclude device caching. fio and own-ring use different random-offset generators,
their timing/instrumentation boundaries differ, configured QD is not a constant
device depth, and shared load affects the measurements. CPU cost and tail
latency remain higher than fio even when throughput is close.

## Three-way latency follow-up — 2026-09-14

This follow-up explicitly compares native Asio, own-ring, and fio latency rather
than inferring it from IOPS. It supplements, not replaces, the earlier checked
64 MiB/two-CPU and 16-owner measurements: file size, content-check policy,
CPU placement and time of day differ.

Both series use a generated 1 GiB file, 4 KiB O_DIRECT reads, CPU `326` only,
one own ring / one native executor / one fio job, a one-second ramp and five-second
measurement, and three repetitions with rotated profile order. All participating
threads share the same single logical-CPU budget; the host and SMT sibling remain
non-exclusive. Raw profiles disable benchmark content comparison but retain
error and length checks. fio uses ordinary files/buffers with batching, without
registered resources, SQPOLL or IOPOLL.

### Same file object

Representative depths are shown below; QD2 and QD8 are also retained in
`results/latency-shared.json`. Every displayed metric is the median of three
per-run values, not a pooled percentile or a selected best run.

| QD | Profile | Median IOPS | Mean us | P50 us | P99 us | P99.9 us |
|---:|---|---:|---:|---:|---:|---:|
| 1 | asio-raw | 11,579 | 86.3 | 63.6 | 176.9 | 2804.6 |
| 1 | own-raw | 14,352 | 69.6 | 57.6 | 124.4 | 2032.7 |
| 1 | own-post-raw | 10,820 | 92.4 | 67.8 | 183.0 | 3265.2 |
| 1 | fio-batch | 11,879 | 83.9 | 60.7 | 187.4 | 3457.0 |
| 32 | asio-raw | 11,449 | 2794.0 | 2461.3 | 8660.5 | 24053.1 |
| 32 | own-raw | 360,288 | 88.8 | 85.0 | 137.2 | 224.3 |
| 32 | own-post-raw | 262,591 | 121.8 | 121.7 | 174.3 | 276.7 |
| 32 | fio-batch | 386,926 | 80.6 | 78.3 | 120.3 | 185.3 |
| 128 | asio-raw | 12,830 | 9964.8 | 7825.2 | 30867.0 | 68333.0 |
| 128 | own-raw | 495,934 | 258.1 | 255.8 | 322.5 | 472.2 |
| 128 | own-post-raw | 427,037 | 299.7 | 295.6 | 360.2 | 447.3 |
| 128 | fio-batch | 485,394 | 202.9 | 193.5 | 309.2 | 370.7 |

### Independent file objects

The C++ backends open 128 independent objects on the same data file, using 32
of them at QD32 and all 128 at QD128. fio retains one file per job. This removes
the native same-object serialization bottleneck without pretending that object
layouts or internal queue depths are identical.

| QD | Profile | Median IOPS | Mean us | P50 us | P99 us | P99.9 us |
|---:|---|---:|---:|---:|---:|---:|
| 32 | asio-raw | 243,936 | 131.1 | 98.3 | 1004.6 | 3099.7 |
| 32 | own-raw | 348,549 | 91.8 | 87.5 | 147.9 | 338.0 |
| 32 | fio-batch | 271,263 | 115.5 | 79.4 | 978.9 | 2007.0 |
| 128 | asio-raw | 454,341 | 281.7 | 279.1 | 344.5 | 733.5 |
| 128 | own-raw | 478,856 | 267.3 | 264.5 | 316.1 | 378.5 |
| 128 | fio-batch | 474,694 | 205.7 | 195.6 | 329.7 | 528.4 |

### What the latency results do and do not establish

- Default own-ring has lower median P99 than native in these tested scenarios.
  However, the independent-object QD128 result is **316.1 vs 344.5 us**, not the
  orders-of-magnitude gap produced by native same-object queueing.
- Default own-ring does **not** always match fio latency. In the same-object
  series at QD32, P99 is **137.2 vs 120.3 us**; at QD128 it is **322.5 vs
  309.2 us**, and P99.9 is **472.2 vs 370.7 us**, despite similar QD128 IOPS.
- Posting resumption has a measurable cost in this configuration. At QD1,
  `own-post-raw` has mean **92.4 vs native 86.3 us**, P99 **183.0 vs 176.9 us**,
  and P99.9 **3265.2 vs 2804.6 us**. Do not substitute default inline-resume
  results for an application that must resume on its executor.
- **Individual runs can regress even when the median improves.** At QD1,
  default own P99 ranges from **91.2 to 243.8 us**, native from **165.8 to
  179.2 us**. Independent-object QD128 own P99 ranges **312.7–403.8 us**, native
  **340.1–363.6 us**. These short, shared-host samples do not establish a
  statistically stable tail-latency advantage. At QD1 the median per-run
  maximum is also worse for own: **12.56 ms vs native 8.00 ms**.
- C++ times the API call through coroutine resumption, including backend
  queueing; fio reports total `lat_ns`, not just `clat_ns`. fio histogram
  quantiles and batching differ from exact C++ samples. Equal configured QD
  does not imply equal average device depth or offered IOPS. These are
  closed-loop tests, **not matched-arrival-rate production SLO measurements**.

All **78 runs / 78,433,810 measured reads** completed without reported I/O errors;
C++ profiles also reported zero short reads. Content comparison is disabled in
these timed runs, so this is not a claim of full payload verification. The
smallest run contains 45,554 samples. Separate checked ON-build smoke tests
cover both backends at QD1 and QD32, with 4096 measured reads and no errors,
short reads or content mismatches.

`results/latency-shared.json` and `results/latency-independent.json` retain
per-run sample counts, mean/P50/P99/P99.9/max, CPU cost, backend identity,
configuration and fio achieved-depth distributions, without private command
paths or hostnames. The comparison script also prints each P99 run range and
the median of per-run maxima. Five Python regression tests cover the metric
parsers, total-latency units, aggregated fio CPU accounting, validation and
median/range summary. Reproduction commands and timing boundaries are in
`README.md` under "Three-way latency comparison".

## Validation

- Default switch OFF and ON: 15 own-ring tests, 1279 assertions passed in each.
- File/shared-handle regression selection: 22 tests, 144994 assertions passed
  in each configuration; existing file error-path tests also passed.
- Clang 17 AddressSanitizer + UndefinedBehaviorSanitizer, including leak checks:
  15 tests and 1279 assertions passed.
- ON benchmark smoke confirms both explicit native and own backends are still
  present; it records `default_own_ring=1` and zero read/verification errors.
- Header compile checks cover the default without io_uring and native-only
  compilation when newer setup-flag declarations are unavailable.
- A separate CMake consumer verifies propagation of the ON compile definition
  and liburing dependency through `yalantinglibs::yalantinglibs`.

Build options, API/lifetime contracts, benchmark commands, and fio reproduction
instructions are in `README.md`. CPU IDs must be adapted to the target machine.
