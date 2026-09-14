import csv
import io
import json
import unittest

from compare_fio import bench_result, fio_result, summarize_results


class ComparisonTest(unittest.TestCase):
    def benchmark_csv(self, **changes):
        row = dict(iops=1000, mean_us=10, p50_us=8, p99_us=20, p999_us=30,
                   max_us=100, cpu_us_per_io=2, operations=5000, seconds=5,
                   verify_data=0, backend="native", files=1, asio_version=102400,
                   post_resume=0, errors=0, short_reads=0, corrupt=0)
        row.update(changes)
        stream = io.StringIO()
        writer = csv.DictWriter(stream, fieldnames=row)
        writer.writeheader()
        writer.writerow(row)
        return stream.getvalue()

    def fio_json(self):
        return dict(jobs=[dict(error=0, usr_cpu=10, sys_cpu=20, job_runtime=10000,
                              iodepth_level={"1": 100}, write=dict(total_ios=0),
                              read=dict(total_ios=10000, runtime=5000, iops=2000,
                                        lat_ns=dict(mean=10000, max=100000,
                                                    percentile={"50.000000": 8000,
                                                                "99.000000": 20000,
                                                                "99.900000": 30000})))])

    def test_benchmark_latency_fields(self):
        result = bench_result(self.benchmark_csv())
        self.assertEqual(result["backend"], "native")
        self.assertEqual(result["runtime_ms"], 5000)
        self.assertEqual(result["p999_us"], 30)
        self.assertEqual(result["mean_us"], 10)
        self.assertEqual(result["max_us"], 100)

    def test_reject_bad_benchmark_samples(self):
        for changes in (dict(errors=1), dict(short_reads=1), dict(corrupt=1),
                        dict(operations=0), dict(seconds=0), dict(p99_us="nan"),
                        dict(mean_us=-1)):
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                bench_result(self.benchmark_csv(**changes))

    def test_fio_uses_total_latency_and_aggregate_cpu_time(self):
        data = self.fio_json()
        data["jobs"][0]["read"]["clat_ns"] = dict(mean=1)
        result = fio_result(json.dumps(data))
        self.assertEqual(result["mean_us"], 10)
        self.assertEqual(result["p999_us"], 30)
        self.assertEqual(result["max_us"], 100)
        self.assertEqual(result["cpu_us_per_io"], 300)

    def test_reject_fio_errors_and_writes(self):
        for field in ("error", "write"):
            data = self.fio_json()
            if field == "error":
                data["jobs"][0][field] = 1
            else:
                data["jobs"][0][field]["total_ios"] = 1
            with self.subTest(field=field), self.assertRaises(ValueError):
                fio_result(json.dumps(data))

    def test_summary_uses_medians_and_retains_tail_range(self):
        rows = []
        for tail in (20, 40, 300):
            row = bench_result(self.benchmark_csv(p99_us=tail))
            row.update(depth=1, jobs=1, profile="asio-raw")
            rows.append(row)
        summary = summarize_results(rows)
        self.assertIn("P99.9 us", summary)
        self.assertIn("| 1 | asio-raw | 1 | 3 |", summary)
        self.assertIn("| 40.0 |", summary)
        self.assertIn("20.0–300.0", summary)


if __name__ == "__main__":
    unittest.main()
