import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path


def summarize(path):
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"{path}: no measurements")
    groups = defaultdict(list)
    for row in rows:
        if any(int(row[key]) for key in ("errors", "short_reads", "corrupt")):
            raise ValueError(f"{path}: failed IO verification: {row}")
        if int(row["operations"]) <= 0 or float(row["seconds"]) <= 0:
            raise ValueError(f"{path}: invalid measurement: {row}")
        key = tuple(row[column] for column in (
            "reactor", "driver", "direct", "read_size", "files", "depth",
            "post_resume", "poll"))
        seconds = float(row.get("seconds_requested", "0"))
        key += (seconds if seconds else row["operations"],
                row.get("verify_data", "1"), row.get("rings", "1"),
                row.get("file_bytes", "unknown"), row.get("pin_owners", "0"))
        groups[key].append(row)
    print(f"## {path.name}\n")
    print("| QD | Backend | Runs | Median IOPS | Speedup | P50 us | P99 us | P99.9 us | CPU us/IO |")
    print("|---:|---|---:|---:|---:|---:|---:|---:|---:|")
    for key, group in groups.items():
        backends = defaultdict(list)
        for row in group:
            backends[row["backend"]].append(row)
        baseline = backends.get("native")
        baseline_iops = statistics.median(float(row["iops"]) for row in baseline) if baseline else None
        for backend, measurements in backends.items():
            medians = {column: statistics.median(float(row[column]) for row in measurements)
                       for column in ("iops", "p50_us", "p99_us", "p999_us", "cpu_us_per_io")}
            speedup = f"{medians['iops'] / baseline_iops:.2f}x" if baseline_iops else "-"
            print(f"| {key[5]} | {backend} | {len(measurements)} | {medians['iops']:,.0f} | "
                  f"{speedup} | {medians['p50_us']:.1f} | {medians['p99_us']:.1f} | "
                  f"{medians['p999_us']:.1f} | {medians['cpu_us_per_io']:.2f} |")
    print()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: python3 summarize.py result.csv [result.csv ...]")
    try:
        for argument in sys.argv[1:]:
            summarize(Path(argument))
    except (ValueError, KeyError, OSError) as error:
        sys.exit(str(error))
