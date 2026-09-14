import argparse
import csv
import hashlib
import io
import json
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


def own_result(text):
    rows = list(csv.DictReader(io.StringIO(text)))
    if len(rows) != 1:
        raise ValueError("expected exactly one own-ring result")
    row = rows[0]
    if any(int(row[key]) for key in ("errors", "short_reads", "corrupt")):
        raise ValueError(f"own-ring IO verification failed: {row}")
    return {"iops": float(row["iops"]), "p50_us": float(row["p50_us"]),
            "p99_us": float(row["p99_us"]), "cpu_us_per_io": float(row["cpu_us_per_io"]),
            "operations": int(row["operations"]), "runtime_ms": float(row["seconds"]) * 1000,
            "verify_data": int(row["verify_data"])}


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
    return {"iops": iops, "p50_us": percentiles["50.000000"] / 1000,
            "p99_us": percentiles["99.000000"] / 1000,
            "cpu_us_per_io": (job["usr_cpu"] + job["sys_cpu"]) * 10 * job["job_runtime"] / read["total_ios"],
            "operations": read["total_ios"], "runtime_ms": read["runtime"],
            "verify_data": 0, "iodepth_level": job["iodepth_level"]}


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
    parser.add_argument("--pin", action="store_true")
    parser.add_argument("--keep-data", action="store_true")
    args = parser.parse_args()
    depths = [int(value) for value in args.depths.split(",")]
    profiles = args.profiles.split(",")
    allowed = {"own-checked", "own-raw", "fio-default", "fio-batch", "fio-files", "fio-registered"}
    if (not set(profiles) <= allowed or len(set(profiles)) != len(profiles)
            or len(set(depths)) != len(depths) or not depths or min(depths) < 1 or max(depths) > 16384
            or args.seconds < 1 or args.seconds > 30 or args.ramp_seconds < 0
            or args.ramp_seconds > 30 or args.repeats < 1 or args.repeats > 20
            or args.file_mib < 1 or args.file_mib > 1024 or args.extra_jobs < 1
            or args.own_rings < 1 or args.own_rings > 64 or min(depths) < args.own_rings
            or args.fio_jobs < 1 or args.fio_jobs > 64
            or any(depth % args.fio_jobs for depth in depths)
            or (args.extra_jobs > 1 and max(depths) % args.extra_jobs)
            or args.extra_jobs > 64 or args.read_size <= 0 or args.read_size % 4096
            or args.read_size > args.file_mib * 1024 * 1024):
        parser.error("invalid comparison parameters")
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
                "own_rings": args.own_rings, "fio_jobs": args.fio_jobs, "pin": args.pin}
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
        if profile.startswith("own-"):
            command = prefix + [str(bench), "--data-file", str(data),
                                "--backends", "defer", "--depths", str(depth),
                                "--repeats", "1", "--seconds", str(args.seconds),
                                "--ramp-seconds", str(args.ramp_seconds),
                                "--rings", str(args.own_rings), "--files", str(args.own_rings),
                                "--read-size", str(args.read_size)]
            if profile == "own-raw":
                command.append("--no-verify")
            if args.pin:
                command.append("--pin-owners")
            output = directory / (stem + ".csv")
            parse = own_result
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
    summary = ["| QD total | Profile | Jobs | Median IOPS | P99 us | Own checked / reference | Own raw / reference |",
               "|---:|---|---:|---:|---:|---:|---:|"]
    for depth in depths:
        checked = [row["iops"] for row in results if row["depth"] == depth and row["profile"] == "own-checked"]
        checked_iops = statistics.median(checked) if checked else None
        raw = [row["iops"] for row in results if row["depth"] == depth and row["profile"] == "own-raw"]
        raw_iops = statistics.median(raw) if raw else None
        keys = dict.fromkeys((row["profile"], row["jobs"]) for row in results if row["depth"] == depth)
        for profile, jobs in keys:
            samples = [row for row in results if row["depth"] == depth and row["profile"] == profile and row["jobs"] == jobs]
            iops = statistics.median(row["iops"] for row in samples)
            latency = statistics.median(row["p99_us"] for row in samples)
            ratio = f"{checked_iops / iops:.1%}" if checked_iops and profile.startswith("fio-") else "-"
            raw_ratio = f"{raw_iops / iops:.1%}" if raw_iops and profile.startswith("fio-") else "-"
            summary.append(f"| {depth} | {profile} | {jobs} | {iops:,.0f} | {latency:.1f} | {ratio} | {raw_ratio} |")
    (directory / "summary.md").write_text("\n".join(summary) + "\n")
    print("\n".join(summary))
    if not args.keep_data:
        data.unlink()


if __name__ == "__main__":
    main()
