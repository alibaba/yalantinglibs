# Own-ring random file reads and benchmarks

## Opt-in backend

`YLT_ENABLE_OWN_RING` defaults to **OFF**. Enabling it changes the default
`random_coro_file` / `basic_random_coro_file<>` read backend to a dedicated
io_uring owner. Random writes still use Asio. Sequential files, sockets, and
the network reactor are unchanged.

```sh
cmake -S . -B build-native -DYLT_ENABLE_FILE_IO_URING=ON -DYLT_ENABLE_OWN_RING=OFF
cmake -S . -B build-own -DYLT_ENABLE_OWN_RING=ON
```

ON automatically enables the file io_uring dependency, not `ASIO_DISABLE_EPOLL`.
The new backend requires Linux and liburing 2.3+. CMake rejects an unsupported
ON configuration; OFF does not require upgrading an existing native-only build.
Without CMake, define `YLT_ENABLE_OWN_RING=1`, `ASIO_HAS_IO_URING`, and
`ASIO_HAS_FILE`, and link `-luring -pthread`. An absent/zero switch keeps the old
default. All translation units must agree on this switch and be rebuilt; this
is a compile-time default-type change, not a runtime/ABI-compatible toggle.

Explicit types are also available with supported file io_uring headers:

- `native_random_coro_file` always selects the original native backend.
- `own_ring_random_coro_file` always selects own-ring reads.
- The benchmark uses these explicit types, so enabling the default switch does
  not accidentally compare own-ring against itself.

The vendored Asio version is recorded as `asio_version` in benchmark CSV output.
No vendored Asio sources are modified.

## Scheduling and lifetime

Asio already uses io_uring `user_data`, but its per-file read queue submits the
head request. This backend associates each independent request with its own
SQE/CQE. One owner thread creates the ring, submits work, reaps completions, and
exits the ring. `SINGLE_ISSUER` is therefore an actual thread-ownership property.

- Coroutine requests live in coroutine frames; `submit_borrowed_read` avoids a
  separate request allocation and request lookup table. `submit_read` retains
  an owning low-level interface.
- Foreign threads hand off through an intrusive queue. Owner-local requests
  bypass that handoff and do not write eventfd on every read.
- Submission is budgeted. Temporary SQ fullness retains pending requests;
  `max_requests` bounds admission and returns `EAGAIN` when exceeded.
- CQ entries are copied and advanced before callbacks execute. A completion may
  resubmit its request, but a borrowed request must not otherwise be moved,
  destroyed, or submitted again before completion.
- Existing shared file handle ownership is retained through completion,
  including when the file wrapper is closed. Buffers must remain valid until
  completion. Iovec descriptors are copied; short-read retries do not mutate
  the caller's descriptors.
- Scalar and vector reads retry partial results. EOF returns success and the
  actual byte count and sets `eof()`, matching the existing native read API.
  Invalid pointers, counts, oversized requests, and offset overflow are rejected.
  A request is limited to `INT_MAX` bytes.
- Terminate signals cooperatively cancel queued work. In-flight work waits for
  its real CQE before returning cancellation; the kernel cannot retain access
  to a buffer already returned to its caller. This does not use async-cancel
  SQEs and does not promise immediate cancellation of a device operation.
- `request_stop()` stops admission, cancels pending work, and drains submitted
  reads. `stop()`/destruction must be serialized on an external thread. Never
  destroy a context or synchronously wait for its own progress inside an owner
  callback. A permanently stuck device can make draining wait indefinitely.

By default callbacks resume inline on the owner. Slow business logic or blocking
work after a read also occupies that owner. `Options::post_resume=true` posts
coroutine resumption to the file's Asio executor instead; that executor must be
running. Asio writes always need a running executor.

## Context configuration

Without an explicit context, opening a file selects an owner from a process-wide
pool. `YLT_OWN_RING_THREADS` sets its size (1–64, default 1) before first use.
Selection is per file, not per read, so a worker can keep its owner-local fast
path. This pool is retained for process lifetime to avoid static-destruction
races with background executors. Use explicit contexts for deterministic cleanup:

```cpp
coro_io::OwnRingIoContext::Options options;
options.entries = 4096;
options.max_requests = 65536;
options.mode = coro_io::OwnRingIoContext::Mode::defer_taskrun;
options.allow_fallback = false;
coro_io::OwnRingIoContext context(options);
if (!context.ok()) {
  throw std::system_error(-context.init_error(), std::system_category());
}
coro_io::own_ring_random_coro_file file(context, executor);
```

`executor` is an `ExecutorWrapper<>*` owned by the caller. The context/executor
must outlive their operations. Normal `open`, shared-handle construction,
`async_read_at`, `async_write_at`, and `close` remain available. The own backend
also exposes `async_readv_at`.

| Option | Default | Meaning |
|---|---:|---|
| `entries` | 4096 | SQ capacity |
| `submit_batch` | 128 | Per-iteration submission budget |
| `max_requests` | 65536 | Pending plus submitted admission limit |
| `mode` | `defer_taskrun` | `plain`, `single_issuer`, or `defer_taskrun` |
| `allow_fallback` | true | Fall back to plain setup on unsupported setup flags |
| `poll` | false | Busy polling instead of blocking |
| `spin_us` | 0 | Optional spin interval before blocking |
| `cpu_id` | -1 | Optional owner affinity |
| `post_resume` | false | Post resumption to Asio rather than resume inline |
| `measure_queue_time` | false | Collect enqueue-to-prepare timing |

The convenience constructor also reads `YLT_OWN_RING_MODE=plain|single|defer`,
`YLT_OWN_RING_POLL`, `YLT_OWN_RING_SPIN_US`, `YLT_OWN_RING_CPU_AFFINITY`, and
`YLT_OWN_RING_POST_RESUME`. Explicit `Options` do not read these variables.

## Build and tests

```sh
cmake -S . -B build-own-ring -DCMAKE_BUILD_TYPE=Release \
  -DYLT_ENABLE_FILE_IO_URING=ON -DYLT_ENABLE_OWN_RING=OFF \
  -DBUILD_EXAMPLES=OFF -DBUILD_UNIT_TESTS=ON -DBUILD_BENCHMARK=ON
cmake --build build-own-ring --target own_ring_test coro_file_bench coro_file_bench_uring -j4
ctest --test-dir build-own-ring -R '^own_ring_test$' --output-on-failure
```

Use a separate ON build to test default-type selection. Tests generate their own
patterned data and cover same-fd concurrency, bounded admission, SQ pressure,
partial scalar/vector reads, EOF, cancellation, callback reentry, buffer/fd
lifetime, shutdown, setup fallback, writes, and shared-handle interoperability.

## Asio comparison

```sh
mkdir -p build-own-ring/results
build-own-ring/output/benchmark/coro_file_bench \
  --backends native,defer --depths 1,32,128 --operations 30000 --repeats 3 \
  > build-own-ring/results/native-vs-own.csv
python3 src/coro_io/benchmark/summarize.py build-own-ring/results/native-vs-own.csv
```

Both backends read identical patterned data with the same offset generator and
validate errors, lengths, and full contents. Three repetitions rotate backend
order. Use `--files 32 --depths 32` to control for Asio's per-object serialization;
use `coro_file_bench_uring` for Asio's default-ring rather than epoll integration.
The latter distinction only applies when global `YLT_ENABLE_IO_URING` is OFF.

`--rings N --files N` uses N explicit owner contexts. Queue depth is total depth.
`--pin-owners` pins owners to ascending CPUs in the current affinity mask; supply
N distinct physical cores using `taskset -c`. `max_read_sqes` is a sum of per-ring
peak outstanding SQEs, not a simultaneous hardware queue depth. `enter_calls`
counts driver calls, not an independently measured syscall count.

Other controls include `--buffered`, `--read-size`, `--driver sync`,
`--post-resume`, `--poll`, `--seconds`, and `--ramp-seconds`. The coroutine driver
allows total QD up to 16384; the thread-based sync driver is capped at 1024.
Timed runs are limited to 30 seconds per measurement/ramp phase.

## fio comparison

```sh
python3 src/coro_io/benchmark/compare_fio.py \
  --bench build-own-ring/output/benchmark/coro_file_bench \
  --output build-own-ring/results/fio-comparison \
  --cpus 0,2,4,6 --own-rings 4 --fio-jobs 4 --pin --extra-jobs 1 \
  --depths 512,2048 --profiles own-raw,fio-batch,own-checked
```

Replace CPU IDs with allowed, distinct cores on the test machine. The output
directory must not exist. Each comparison creates a non-sparse 1 GiB file shared
by both tools, with 4 KiB O_DIRECT reads, a one-second ramp, five-second measured
phase, and three repetitions by default. All fio commands use `--readonly` and
`--allow_file_create=0`; no existing application files or block devices are
written. Successful runs delete their generated data unless `--keep-data` is set.

- `own-checked` compares every returned buffer with its expected contents.
- `own-raw` only skips this benchmark comparison, not API error/length checks.
- `asio-raw` and `asio-checked` select the explicit native Asio backend, with
  the same error/length/content-check policy as their own-ring counterparts.
- `own-post-raw` additionally posts coroutine resumption to the Asio executor;
  compare it with `own-raw` to expose scheduling costs rather than treating
  the default inline-resume path as universally appropriate for applications.
- `fio-batch` uses io_uring, ordinary fd/buffers, non-vectored reads, batched
  submission/completion, and no SQPOLL/IOPOLL.
- `fio-files` adds registered files; optional `fio-registered` also registers
  buffers. Unsupported settings fail instead of being silently downgraded.
- Default runs add a two-job fio control at maximum total depth. Use
  `--extra-jobs 1` to disable it and `--fio-jobs N` for matched multi-ring tests.

fio and own-ring share the file/range, but not an identical random-offset replay.
Their timing boundaries and instrumentation costs differ. CPU cost uses fio's
aggregated job runtime rather than treating averaged job CPU percentages as a
process total. Results retain commands, versions, binary SHA256, inode, and raw
CSV/JSON locally; review machine-specific metadata before publishing it.

A throughput ratio against fio is not a physical-device utilization percentage.
Shared load, CPU/NUMA placement, working-set size, device caches, and configured
versus actual queue depth matter. See `RESULTS.md` for the public-upstream test
configuration, results, and explicitly limited interpretation.

### Three-way latency comparison

```sh
python3 src/coro_io/benchmark/compare_fio.py \
  --bench build-own-ring/output/benchmark/coro_file_bench \
  --output build-own-ring/results/latency-shared \
  --cpus 0 --own-rings 1 --fio-jobs 1 --pin --extra-jobs 1 \
  --depths 1,2,8,32,128 \
  --profiles asio-raw,own-raw,own-post-raw,fio-batch

python3 src/coro_io/benchmark/compare_fio.py \
  --bench build-own-ring/output/benchmark/coro_file_bench \
  --output build-own-ring/results/latency-independent \
  --cpus 0 --own-rings 1 --fio-jobs 1 --pin --extra-jobs 1 \
  --files 128 --depths 32,128 --profiles asio-raw,own-raw,fio-batch

python3 -m unittest discover -s src/coro_io/benchmark -p 'test_*.py'
```

Use an allowed CPU on the target machine. All participating threads share this
one-CPU budget, including the own-ring owner and the C++ executor thread. CPU
affinity is not CPU reservation; other tasks and SMT siblings may interfere.
Asio profiles require one own ring and one fio job because the native benchmark
has one executor. `--files` controls the number of independent C++ file objects
opened on the same data file; only `min(files, QD)` objects are actively used.
fio still uses one file per job, so the second command is an application-object
serialization control, not an identical file-descriptor layout.

The comparison records mean, P50, P99, P99.9, maximum latency, IOPS and CPU cost.
All summary metrics are medians of per-run values, including the maximum;
the additional P99 range retains the smallest and largest per-run P99. These
are not pooled percentiles or confidence intervals. Raw per-run values and
sample counts remain in `measurements.json`; a small sample count makes P99.9
and maximum particularly unstable. `summarize.py` also retains P99.9 when
processing the standalone benchmark CSV, including older result files.

Timing boundaries:

- Both C++ backends measure from immediately before `async_read_at` until
  the awaiting worker resumes. This includes backend queueing and coroutine
  resumption, but excludes offset generation and the content comparison after
  the read. Content comparison still affects throughput and other workers.
- fio uses total `lat_ns` (submission plus completion latency), not `clat_ns`
  alone. Its instrumentation boundary and histogram quantiles differ from
  the C++ per-operation clock samples. This is an application-API versus fio
  reference comparison, not an isolated device-service-time comparison.
- Workloads are closed-loop: at most QD requests are outstanding, and each
  worker submits again after completion. Equal QD does not mean equal offered
  IOPS or equal device depth. This does not measure latency at a fixed arrival
  rate, open-loop overload, coordinated-omission-corrected latency, or a
  production SLO. In particular, same-object Asio queueing at high QD must
  not be used to claim a universal per-I/O latency improvement.

Start with QD1/2 to assess low-concurrency regressions, then inspect the
independent-object control and P99/P99.9 variability. `fio-default` can replace
`fio-batch` to disable batching beyond one request when investigating batching
tradeoffs; keep the chosen profile explicit in reports.
