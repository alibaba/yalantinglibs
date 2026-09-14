import argparse
import csv
import hashlib
import io
import json
import math
import os
import platform
import statistics
import subprocess
import time
from pathlib import Path


def run(command, directory, output, timeout):
    result = subprocess.run(command, cwd=directory, text=True, capture_output=True,
                            timeout=timeout)
    output.write_text(result.stdout)
    output.with_suffix(output.suffix + ".stderr").write_text(result.stderr)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {command}\n{result.stderr}\n{result.stdout[:300]}")
    return result.stdout


def bench_result(text):
    rows = list(csv.DictReader(io.StringIO(text)))
    if len(rows) != 1:
        raise ValueError("expected exactly one benchmark result")
    row = rows[0]
    if any(int(row[key]) for key in ("errors", "short_reads", "corrupt")):
        raise ValueError(f"benchmark IO verification failed: {row}")
    measured = {key: float(row[key]) for key in
                ("iops", "mean_us", "p50_us", "p99_us", "p999_us", "max_us", "cpu_us_per_io")}
    measured.update(operations=int(row["operations"]), runtime_ms=float(row["seconds"]) * 1000,
                    verify_data=int(row["verify_data"]), backend=row["backend"],
                    files=int(row["files"]), asio_version=int(row["asio_version"]),
                    post_resume=int(row["post_resume"]),
                    latency_scope="async_read_at call through coroutine resume; before content check")
    validate_result(measured)
    return measured


def fio_result(text):
    jobs = json.loads(text)["jobs"]
    if len(jobs) != 1 or jobs[0]["error"]:
        raise ValueError("fio failed or did not aggregate jobs")
    job = jobs[0]
    read = job["read"]
    if not read["total_ios"] or job["write"]["total_ios"]:
        raise ValueError("fio did not perform a read-only workload")
    percentiles = read["lat_ns"]["percentile"]
    iops = read["iops"]
    measured = {"iops": iops, "mean_us": read["lat_ns"]["mean"] / 1000,
            "max_us": read["lat_ns"]["max"] / 1000,
            "p50_us": percentiles["50.000000"] / 1000,
            "p99_us": percentiles["99.000000"] / 1000,
            "p999_us": percentiles["99.900000"] / 1000,
            "cpu_us_per_io": (job["usr_cpu"] + job["sys_cpu"]) * 10 * job["job_runtime"] / read["total_ios"],
            "operations": read["total_ios"], "runtime_ms": read["runtime"],
            "verify_data": 0, "iodepth_level": job["iodepth_level"],
            "latency_scope": "fio total lat (slat + clat), not clat alone"}
    validate_result(measured)
    return measured


def validate_result(measured):
    for key in ("iops", "mean_us", "p50_us", "p99_us", "p999_us", "max_us",
                "cpu_us_per_io", "operations", "runtime_ms"):
        if not math.isfinite(measured[key]) or measured[key] < 0:
            raise ValueError(f"invalid {key}: {measured[key]}")
    if not measured["operations"] or not measured["runtime_ms"] or not measured["iops"]:
        raise ValueError("empty measurement")


def summarize_results(results):
    summary = ["| QD total | Profile | Jobs | Runs | Median IOPS | Mean us | P50 us | P99 us | P99.9 us | Median max us | P99 run range us | CPU us/IO |",
               "|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|"]
    keys = dict.fromkeys((row["depth"], row["profile"], row["jobs"]) for row in results)
    for depth, profile, jobs in keys:
        samples = [row for row in results if (row["depth"], row["profile"], row["jobs"]) == (depth, profile, jobs)]
        medians = {key: statistics.median(row[key] for row in samples) for key in
                   ("iops", "mean_us", "p50_us", "p99_us", "p999_us", "max_us", "cpu_us_per_io")}
        tails = [row["p99_us"] for row in samples]
        summary.append(f"| {depth} | {profile} | {jobs} | {len(samples)} | {medians['iops']:,.0f} | "
                       f"{medians['mean_us']:.1f} | {medians['p50_us']:.1f} | {medians['p99_us']:.1f} | "
                       f"{medians['p999_us']:.1f} | {medians['max_us']:.1f} | "
                       f"{min(tails):.1f}–{max(tails):.1f} | {medians['cpu_us_per_io']:.2f} |")
    return "\n".join(summary) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bench", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cpus", required=True)
    parser.add_argument("--depths", default="1,32,128,512")
    parser.add_argument("--profiles", default="own-checked,own-raw,fio-batch,fio-files")
    parser.add_argument("--seconds", type=int, default=5)
    parser.add_argument("--ramp-seconds", type=int, default=1)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--file-mib", type=int, default=1024)
    parser.add_argument("--read-size", type=int, default=4096)
    parser.add_argument("--extra-jobs", type=int, default=2)
    parser.add_argument("--own-rings", type=int, default=1)
    parser.add_argument("--fio-jobs", type=int, default=1)
    parser.add_argument("--files", type=int, help="file objects for each C++ backend; defaults to own-rings")
    parser.add_argument("--pin", action="store_true")
    parser.add_argument("--keep-data", action="store_true")
    args = parser.parse_args()
    if args.files is None:
        args.files = args.own_rings
    depths = [int(value) for value in args.depths.split(",")]
    profiles = args.profiles.split(",")
    allowed = {"own-checked", "own-raw", "own-post-raw", "asio-checked", "asio-raw",
               "fio-default", "fio-batch", "fio-files", "fio-registered"}
    if (not set(profiles) <= allowed or len(set(profiles)) != len(profiles)
            or len(set(depths)) != len(depths) or not depths or min(depths) < 1 or max(depths) > 16384
            or args.seconds < 1 or args.seconds > 30 or args.ramp_seconds < 0
            or args.ramp_seconds > 30 or args.repeats < 1 or args.repeats > 20
            or args.file_mib < 1 or args.file_mib > 1024 or args.extra_jobs < 1
            or args.own_rings < 1 or args.own_rings > 64 or min(depths) < args.own_rings
            or args.fio_jobs < 1 or args.fio_jobs > 64
            or args.files < args.own_rings or args.files > 256
            or any(depth % args.fio_jobs for depth in depths)
            or (args.extra_jobs > 1 and max(depths) % args.extra_jobs)
            or args.extra_jobs > 64 or args.read_size <= 0 or args.read_size % 4096
            or args.read_size > args.file_mib * 1024 * 1024):
        parser.error("invalid comparison parameters")
    if any(profile.startswith("asio-") for profile in profiles) and (args.own_rings != 1 or args.fio_jobs != 1):
        parser.error("Asio comparison requires one own ring and one fio job: native uses one executor")
    bench = args.bench.resolve(strict=True)
    directory = args.output.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    data = directory / "data.bin"
    commands = []
    metadata = {"time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
                "benchmark_sha256": hashlib.sha256(bench.read_bytes()).hexdigest(),
                "comparison_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "kernel": {"system": platform.system(), "release": platform.release(),
                           "machine": platform.machine()}, "affinity": args.cpus,
                "fio_version": subprocess.check_output(["fio", "--version"], text=True).strip(),
                "seconds": args.seconds, "ramp_seconds": args.ramp_seconds,
                "file_mib": args.file_mib, "read_size": args.read_size,
                "loadavg": os.getloadavg(), "profiles": profiles, "depths": depths,
                "own_rings": args.own_rings, "fio_jobs": args.fio_jobs, "pin": args.pin,
                "files": args.files, "load_model": "closed-loop fixed maximum outstanding requests"}
    command = [str(bench), "--prepare-file", str(data), "--file-mib", str(args.file_mib)]
    commands.append(command)
    run(command, directory, directory / "prepare.log", 180)
    metadata["file_inode"] = data.stat().st_ino
    metadata["file_bytes"] = data.stat().st_size
    (directory / "environment.json").write_text(json.dumps(metadata, indent=2))
    results = []

    def measure(profile, depth, repeat, jobs=1):
        stem = f"{profile}-qd{depth}-jobs{jobs}-r{repeat}"
        print(stem, flush=True)
        prefix = ["taskset", "-c", args.cpus]
        if profile.startswith(("own-", "asio-")):
            command = prefix + [str(bench), "--data-file", str(data),
                                "--backends", "native" if profile.startswith("asio-") else "defer", "--depths", str(depth),
                                "--repeats", "1", "--seconds", str(args.seconds),
                                "--ramp-seconds", str(args.ramp_seconds),
                                "--rings", str(args.own_rings), "--files", str(args.files),
                                "--read-size", str(args.read_size)]
            if profile.endswith("-raw"):
                command.append("--no-verify")
            if profile == "own-post-raw":
                command.append("--post-resume")
            if args.pin:
                command.append("--pin-owners")
            output = directory / (stem + ".csv")
            parse = bench_result
        else:
            per_job = depth // jobs
            if per_job * jobs != depth:
                raise ValueError("total depth must be divisible by jobs")
            batch = 1 if profile == "fio-default" else min(32, per_job)
            complete = 1 if profile == "fio-default" else per_job
            fixed_buffers = int(profile == "fio-registered")
            registered_files = int(profile in {"fio-files", "fio-registered"})
            command = prefix + ["fio", "--readonly", "--allow_file_create=0",
                "--name=ring-reference", "--ioengine=io_uring", "--thread=1",
                "--rw=randread", "--direct=1", "--nonvectored=1",
                f"--filename={data}", f"--size={data.stat().st_size}",
                f"--bs={args.read_size}", f"--iodepth={per_job}", f"--numjobs={jobs}",
                f"--iodepth_batch_submit={batch}", "--iodepth_batch_complete_min=1",
                f"--iodepth_batch_complete_max={complete}",
                f"--fixedbufs={fixed_buffers}", f"--registerfiles={registered_files}",
                "--hipri=0", "--sqthread_poll=0", "--norandommap=1",
                "--randrepeat=1", "--randseed=24680", "--time_based=1",
                f"--runtime={args.seconds}", f"--ramp_time={args.ramp_seconds}",
                "--lat_percentiles=1", "--clat_percentiles=0", "--slat_percentiles=0",
                "--percentile_list=50:99:99.9", "--group_reporting=1",
                "--output-format=json", "--eta=never"]
            if args.pin:
                command.extend([f"--cpus_allowed={args.cpus}", "--cpus_allowed_policy=split"])
            output = directory / (stem + ".json")
            parse = fio_result
        commands.append(command)
        (directory / "commands.json").write_text(json.dumps(commands, indent=2))
        started = time.time()
        measured = parse(run(command, directory, output, args.seconds + args.ramp_seconds + 60))
        measured.update(profile=profile, depth=depth, repeat=repeat, jobs=jobs,
                        started_unix=started, finished_unix=time.time())
        results.append(measured)
        (directory / "measurements.json").write_text(json.dumps(results, indent=2))

    for depth in depths:
        for repeat in range(args.repeats):
            for index in range(len(profiles)):
                profile = profiles[(index + repeat) % len(profiles)]
                measure(profile, depth, repeat, args.own_rings if profile.startswith("own-") else args.fio_jobs)
    if args.extra_jobs > 1:
        reference = "fio-registered" if "fio-registered" in profiles else "fio-files" if "fio-files" in profiles else "fio-batch"
        for repeat in range(args.repeats):
            measure(reference, max(depths), repeat, args.extra_jobs)
    summary = summarize_results(results)
    (directory / "summary.md").write_text(summary)
    print(summary)
    if not args.keep_data:
        data.unlink()


if __name__ == "__main__":
    main()
