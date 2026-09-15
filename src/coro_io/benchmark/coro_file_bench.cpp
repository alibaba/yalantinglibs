#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>

#include <algorithm>
#include <array>
#include <asio/version.hpp>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <memory>
#include <numeric>
#include <string_view>
#include <thread>
#include <vector>
#include <ylt/coro_io/coro_file.hpp>

#include "../tests/file_io_fixture.hpp"

using Clock = std::chrono::steady_clock;
using async_simple::coro::Lazy;
using async_simple::coro::syncAwait;
using file_io_test::block_size;

struct Configuration {
  size_t operations = 20000;
  size_t repeats = 3;
  size_t file_mib = 64;
  bool inline_resume = false;
};

struct ExecutorThread {
  asio::io_context context{1};
  coro_io::ExecutorWrapper<> executor{context.get_executor()};
  asio::executor_work_guard<asio::io_context::executor_type> guard{
      context.get_executor()};
  std::thread thread{[this] {
    context.run();
  }};
  ~ExecutorThread() {
    guard.reset();
    context.stop();
    thread.join();
  }
};

struct Worker {
  file_io_test::AlignedBuffer buffer;
  std::vector<double> latencies;
  size_t errors = 0;
  size_t short_reads = 0;
  size_t corrupt = 0;
  explicit Worker(size_t samples) { latencies.reserve(samples); }
  void reset() {
    latencies.clear();
    errors = short_reads = corrupt = 0;
  }
};

struct Measurement {
  double iops;
  double mean;
  double p50;
  double p99;
  size_t errors;
  size_t short_reads;
  size_t corrupt;
};

template <typename File>
Lazy<void> read_worker(File &file, Worker &worker, size_t worker_id,
                       size_t operations, size_t blocks) {
  for (size_t iteration = 0; iteration < operations; ++iteration) {
    size_t block = (iteration * 977 + worker_id * 131) % blocks;
    auto start = Clock::now();
    auto result = co_await file.async_read_at(block * block_size,
                                              worker.buffer.data(), block_size);
    worker.latencies.push_back(
        std::chrono::duration<double, std::micro>(Clock::now() - start)
            .count());
    worker.errors += bool(result.first);
    worker.short_reads += result.second != block_size;
    if (!result.first && result.second == block_size &&
        !file_io_test::verify(worker.buffer.data(), block, block_size)) {
      ++worker.corrupt;
    }
  }
}

template <typename File>
Lazy<void> read_all(std::vector<std::unique_ptr<File>> &files,
                    std::vector<std::unique_ptr<Worker>> &workers,
                    size_t operations, size_t blocks) {
  std::vector<Lazy<void>> tasks;
  for (size_t index = 0; index < workers.size(); ++index) {
    size_t count = operations / workers.size() +
                   size_t(index < operations % workers.size());
    tasks.push_back(read_worker(*files[index % files.size()], *workers[index],
                                index, count, blocks));
  }
  auto results = co_await async_simple::coro::collectAll(std::move(tasks));
  for (auto &result : results) {
    result.value();
  }
}

template <typename File>
Measurement measure(const Configuration &config,
                    const file_io_test::TemporaryFile &data, size_t file_count,
                    size_t depth) {
  std::unique_ptr<coro_io::OwnRingIoContext> ring;
  if constexpr (std::is_same_v<File, coro_io::own_ring_random_coro_file>) {
    coro_io::OwnRingIoContext::Options options;
    options.post_resume = !config.inline_resume;
    ring = std::make_unique<coro_io::OwnRingIoContext>(options);
    if (!ring->ok()) {
      throw std::system_error(-ring->init_error(), std::system_category());
    }
  }
  ExecutorThread execution;
  std::vector<std::unique_ptr<File>> files;
  for (size_t index = 0; index < file_count; ++index) {
    if constexpr (std::is_same_v<File, coro_io::own_ring_random_coro_file>) {
      files.emplace_back(std::make_unique<File>(*ring, &execution.executor));
    }
    else {
      files.emplace_back(std::make_unique<File>(&execution.executor));
    }
    if (!files.back()->open(data.path(), std::ios::in, true)) {
      throw std::system_error(files.back()->open_error(),
                              "open benchmark file");
    }
  }
  std::vector<std::unique_ptr<Worker>> workers;
  for (size_t index = 0; index < depth; ++index) {
    workers.emplace_back(
        std::make_unique<Worker>(config.operations / depth + 1));
  }
  syncAwait(read_all(files, workers, depth * 8, data.blocks())
                .via(&execution.executor));
  for (auto &worker : workers) {
    if (worker->errors || worker->short_reads || worker->corrupt) {
      throw std::runtime_error("warmup verification failed");
    }
    worker->reset();
  }
  auto start = Clock::now();
  syncAwait(read_all(files, workers, config.operations, data.blocks())
                .via(&execution.executor));
  double seconds = std::chrono::duration<double>(Clock::now() - start).count();
  Measurement measurement{};
  std::vector<double> latencies;
  latencies.reserve(config.operations);
  for (const auto &worker : workers) {
    latencies.insert(latencies.end(), worker->latencies.begin(),
                     worker->latencies.end());
    measurement.errors += worker->errors;
    measurement.short_reads += worker->short_reads;
    measurement.corrupt += worker->corrupt;
  }
  if (latencies.size() != config.operations) {
    throw std::runtime_error("incorrect measurement count");
  }
  std::sort(latencies.begin(), latencies.end());
  measurement.iops = config.operations / seconds;
  measurement.mean = std::accumulate(latencies.begin(), latencies.end(), 0.0) /
                     latencies.size();
  measurement.p50 = latencies[(latencies.size() - 1) / 2];
  measurement.p99 =
      latencies[static_cast<size_t>(0.99 * (latencies.size() - 1))];
  return measurement;
}

double median(const std::vector<Measurement> &samples,
              double Measurement::*field) {
  std::vector<double> values;
  for (const auto &sample : samples) {
    values.push_back(sample.*field);
  }
  std::sort(values.begin(), values.end());
  return (values[(values.size() - 1) / 2] + values[values.size() / 2]) / 2;
}

bool print_summary(const char *scenario, const char *backend, size_t depth,
                   const std::vector<Measurement> &samples) {
  size_t errors = 0;
  size_t short_reads = 0;
  size_t corrupt = 0;
  for (const auto &sample : samples) {
    errors += sample.errors;
    short_reads += sample.short_reads;
    corrupt += sample.corrupt;
  }
  std::printf("%-12s %-12s %4zu %12.0f %12.2f %12.2f %12.2f %8zu %8zu %8zu\n",
              scenario, backend, depth, median(samples, &Measurement::iops),
              median(samples, &Measurement::mean),
              median(samples, &Measurement::p50),
              median(samples, &Measurement::p99), errors, short_reads, corrupt);
  std::fflush(stdout);
  return errors == 0 && short_reads == 0 && corrupt == 0;
}

size_t number(std::string_view text) {
  size_t value = 0;
  auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument("expected a positive integer");
  }
  return value;
}

int main(int argc, char **argv) {
  try {
    Configuration config;
    for (int index = 1; index < argc; ++index) {
      std::string_view option(argv[index]);
      if (option == "--help") {
        std::puts(
            "coro_file_bench [--operations N] [--repeats N] [--file-mib N] "
            "[--inline-resume]\n"
            "Runs shared-object QD1/32/128 and independent-object QD32/128.\n"
            "All reads use 4 KiB O_DIRECT with full content verification.\n"
            "IOPS and latency are medians of per-run values; errors are "
            "totals.\n"
            "Latency includes API queueing and resumption, before content "
            "checks.\n"
            "Fixed-QD closed-loop measurements do not establish production "
            "SLOs.");
        return 0;
      }
      if (option == "--inline-resume") {
        config.inline_resume = true;
        continue;
      }
      if (++index == argc) {
        throw std::invalid_argument("missing option value");
      }
      if (option == "--operations") {
        config.operations = number(argv[index]);
      }
      else if (option == "--repeats") {
        config.repeats = number(argv[index]);
      }
      else if (option == "--file-mib") {
        config.file_mib = number(argv[index]);
      }
      else {
        throw std::invalid_argument("unknown option");
      }
    }
    if (config.operations < 128 || config.operations > 2000000 ||
        !config.repeats || config.repeats > 20 || !config.file_mib ||
        config.file_mib > 1024) {
      throw std::invalid_argument(
          "operations: 128..2000000, repeats: 1..20, file-mib: 1..1024");
    }
    file_io_test::TemporaryFile data(config.file_mib * 1024 * 1024 /
                                     block_size);
    std::printf(
        "Asio=%u, operations/run=%zu, repeats=%zu, file=%zu MiB, own "
        "resume=%s\n",
        unsigned(ASIO_VERSION), config.operations, config.repeats,
        config.file_mib,
        config.inline_resume ? "inline (explicit opt-in)"
                             : "executor (default)");
    std::puts(
        "IOPS/latency: per-run medians; errors/short/corrupt: totals across "
        "runs.");
    std::printf("%-12s %-12s %4s %12s %12s %12s %12s %8s %8s %8s\n", "Scenario",
                "Backend", "QD", "IOPS", "Mean(us)", "P50(us)", "P99(us)",
                "Errors", "Short", "Corrupt");
    bool passed = true;
    for (auto [file_count, depth] : std::array<std::pair<size_t, size_t>, 5>{
             {{1, 1}, {1, 32}, {1, 128}, {32, 32}, {128, 128}}}) {
      std::array<std::vector<Measurement>, 2> samples;
      for (size_t repeat = 0; repeat < config.repeats; ++repeat) {
        for (size_t index = 0; index < samples.size(); ++index) {
          size_t backend = (index + repeat) % samples.size();
          samples[backend].push_back(
              backend == 0 ? measure<coro_io::native_random_coro_file>(
                                 config, data, file_count, depth)
                           : measure<coro_io::own_ring_random_coro_file>(
                                 config, data, file_count, depth));
        }
      }
      const char *scenario = file_count == 1 ? "shared" : "independent";
      passed &= print_summary(scenario, "asio", depth, samples[0]);
      passed &= print_summary(
          scenario, config.inline_resume ? "own(inline)" : "own(post)", depth,
          samples[1]);
    }
    return passed ? 0 : 1;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
