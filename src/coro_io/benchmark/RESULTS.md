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
