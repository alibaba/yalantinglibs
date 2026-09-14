#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>
#include <sys/resource.h>

#include <algorithm>
#include <asio/version.hpp>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <future>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <ylt/coro_io/coro_file.hpp>

#include "../tests/file_io_fixture.hpp"

using coro_io::OwnRingIoContext;
using file_io_test::block_size;
using Clock = std::chrono::steady_clock;
using async_simple::coro::Lazy;
using async_simple::coro::syncAwait;

struct Configuration {
  size_t operations = 20000;
  size_t file_mib = 64;
  size_t read_size = block_size;
  size_t files = 1;
  unsigned rings = 1;
  unsigned repeats = 3;
  bool direct = true;
  bool sync_driver = false;
  bool post_resume = false;
  bool poll = false;
  bool verify_data = true;
  bool pin_owners = false;
  double seconds = 0;
  double ramp_seconds = 0;
  std::string data_file;
  std::string prepare_file;
  std::vector<unsigned> depths{1, 8, 32, 128};
  std::vector<std::string> backends{"native", "plain", "single", "defer"};
};

struct DataSet {
  std::string path;
  size_t blocks;
};

struct Worker {
  file_io_test::AlignedBuffer buffer;
  std::vector<double> latencies;
  size_t errors = 0;
  size_t short_reads = 0;
  size_t corrupt = 0;
  explicit Worker(size_t bytes, size_t samples) : buffer(bytes) {
    latencies.reserve(samples);
  }
  void reset() {
    latencies.clear();
    errors = short_reads = corrupt = 0;
  }
};

class ExecutorThread {
 public:
  asio::io_context context{1};
  coro_io::ExecutorWrapper<> executor{context.get_executor()};
  asio::executor_work_guard<asio::io_context::executor_type> work{
      context.get_executor()};
  std::thread thread{[this] {
    context.run();
  }};
  ~ExecutorThread() {
    work.reset();
    context.stop();
    thread.join();
  }
};

size_t block_for(size_t iteration, size_t worker, size_t blocks,
                 size_t read_size) {
  return (iteration * 977 + worker * 131) %
         (blocks - read_size / block_size + 1);
}

void record(Worker &worker, const std::pair<std::error_code, size_t> &result,
            size_t block, size_t read_size, Clock::time_point start,
            bool verify_data) {
  worker.latencies.push_back(
      std::chrono::duration<double, std::micro>(Clock::now() - start).count());
  worker.errors += bool(result.first);
  worker.short_reads += result.second != read_size;
  if (verify_data && !result.first && result.second == read_size &&
      !file_io_test::verify(worker.buffer.data(), block, read_size)) {
    ++worker.corrupt;
  }
}

template <typename File>
Lazy<void> read_worker(File &file, Worker &worker, size_t worker_id,
                       size_t operations, size_t blocks, size_t read_size,
                       Clock::time_point deadline, bool verify_data) {
  for (size_t iteration = 0; iteration < operations; ++iteration) {
    size_t block = block_for(iteration, worker_id, blocks, read_size);
    auto start = Clock::now();
    if (start >= deadline)
      break;
    auto result = co_await file.async_read_at(block * block_size,
                                              worker.buffer.data(), read_size);
    record(worker, result, block, read_size, start, verify_data);
  }
}

template <typename File>
Lazy<void> read_all(std::vector<std::unique_ptr<File>> &files,
                    std::vector<std::unique_ptr<Worker>> &workers,
                    size_t operations, size_t blocks, size_t read_size,
                    Clock::time_point deadline, bool verify_data) {
  std::vector<Lazy<void>> tasks;
  for (size_t index = 0; index < workers.size(); ++index) {
    size_t count =
        operations / workers.size() + (index < operations % workers.size());
    tasks.push_back(read_worker(*files[index % files.size()], *workers[index],
                                index, count, blocks, read_size, deadline,
                                verify_data));
  }
  auto results = co_await async_simple::coro::collectAll(std::move(tasks));
  for (auto &result : results) {
    result.value();
  }
}

template <typename File>
void drive(const Configuration &config,
           std::vector<std::unique_ptr<File>> &files,
           std::vector<std::unique_ptr<Worker>> &workers,
           coro_io::ExecutorWrapper<> &executor, size_t operations,
           size_t blocks, double seconds = 0) {
  auto deadline =
      seconds > 0 ? Clock::now() + std::chrono::duration_cast<Clock::duration>(
                                       std::chrono::duration<double>(seconds))
                  : Clock::time_point::max();
  if (seconds > 0)
    operations = std::numeric_limits<size_t>::max() / 2;
  if (!config.sync_driver) {
    syncAwait(read_all(files, workers, operations, blocks, config.read_size,
                       deadline, config.verify_data)
                  .via(&executor));
    return;
  }
  std::vector<std::future<void>> futures;
  for (size_t index = 0; index < workers.size(); ++index) {
    futures.push_back(std::async(std::launch::async, [&, index] {
      size_t count =
          operations / workers.size() + (index < operations % workers.size());
      auto &file = *files[index % files.size()];
      auto &worker = *workers[index];
      for (size_t iteration = 0; iteration < count; ++iteration) {
        size_t block = block_for(iteration, index, blocks, config.read_size);
        auto start = Clock::now();
        if (start >= deadline)
          break;
        auto result =
            syncAwait(file.async_read_at(block * block_size,
                                         worker.buffer.data(), config.read_size)
                          .via(&executor));
        record(worker, result, block, config.read_size, start,
               config.verify_data);
      }
    }));
  }
  for (auto &future : futures) {
    future.get();
  }
}

double cpu_seconds(const rusage &usage) {
  return usage.ru_utime.tv_sec + usage.ru_utime.tv_usec * 1e-6 +
         usage.ru_stime.tv_sec + usage.ru_stime.tv_usec * 1e-6;
}

template <typename File>
bool benchmark(const Configuration &config, const DataSet &data,
               const std::string &backend, unsigned depth, unsigned repeat) {
  std::vector<std::unique_ptr<OwnRingIoContext>> rings;
  if constexpr (std::is_same_v<File, coro_io::own_ring_random_coro_file>) {
    OwnRingIoContext::Options options;
    options.mode = backend == "plain" ? OwnRingIoContext::Mode::plain
                   : backend == "single"
                       ? OwnRingIoContext::Mode::single_issuer
                       : OwnRingIoContext::Mode::defer_taskrun;
    options.allow_fallback = false;
    options.poll = config.poll;
    options.post_resume = config.post_resume;
    std::vector<int> owner_cpus;
    if (config.pin_owners) {
      cpu_set_t available;
      if (::sched_getaffinity(0, sizeof(available), &available) < 0) {
        throw std::system_error(errno, std::system_category(), "get affinity");
      }
      for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &available))
          owner_cpus.push_back(cpu);
      }
      if (owner_cpus.size() < config.rings)
        throw std::invalid_argument(
            "each owner requires a distinct allowed CPU");
    }
    for (unsigned index = 0; index < config.rings; ++index) {
      if (config.pin_owners)
        options.cpu_id = owner_cpus[index];
      rings.emplace_back(std::make_unique<OwnRingIoContext>(options));
      if (!rings.back()->ok()) {
        throw std::system_error(-rings.back()->init_error(),
                                std::system_category(), backend);
      }
    }
  }
  auto statistics = [&] {
    OwnRingIoContext::Statistics total{};
    for (const auto &ring : rings) {
      auto current = ring->statistics();
      total.accepted += current.accepted;
      total.completed += current.completed;
      total.rejected += current.rejected;
      total.wake_writes += current.wake_writes;
      total.enter_calls += current.enter_calls;
      total.read_sqes += current.read_sqes;
      total.max_inflight += current.max_inflight;
      total.queue_ns += current.queue_ns;
      total.outstanding += current.outstanding;
    }
    return total;
  };
  ExecutorThread execution;
  std::vector<std::unique_ptr<File>> files;
  for (size_t index = 0; index < config.files; ++index) {
    if constexpr (std::is_same_v<File, coro_io::own_ring_random_coro_file>) {
      files.emplace_back(std::make_unique<File>(*rings[index % rings.size()],
                                                &execution.executor));
    }
    else {
      files.emplace_back(std::make_unique<File>(&execution.executor));
    }
    if (!files.back()->open(data.path, std::ios::in, config.direct)) {
      throw std::runtime_error("file open failed: " + backend);
    }
  }
  std::vector<std::unique_ptr<Worker>> workers;
  for (unsigned index = 0; index < depth; ++index) {
    workers.emplace_back(std::make_unique<Worker>(
        config.read_size,
        std::max<size_t>(
            128,
            config.seconds > 0
                ? static_cast<size_t>(
                      std::max(config.seconds, config.ramp_seconds) * 2000000) /
                          depth +
                      1
                : config.operations / depth + 1)));
  }
  drive(config, files, workers, execution.executor, depth * 64, data.blocks,
        config.ramp_seconds);
  for (auto &worker : workers) {
    if (worker->errors || worker->short_reads || worker->corrupt) {
      throw std::runtime_error("warmup verification failed: " + backend);
    }
    worker->reset();
  }
  auto before = statistics();
  rusage cpu_before{}, cpu_after{};
  ::getrusage(RUSAGE_SELF, &cpu_before);
  auto start = Clock::now();
  drive(config, files, workers, execution.executor, config.operations,
        data.blocks, config.seconds);
  double seconds = std::chrono::duration<double>(Clock::now() - start).count();
  ::getrusage(RUSAGE_SELF, &cpu_after);
  auto after = statistics();
  std::vector<double> latencies;
  size_t errors = 0, short_reads = 0, corrupt = 0;
  for (auto &worker : workers) {
    latencies.insert(latencies.end(), worker->latencies.begin(),
                     worker->latencies.end());
    errors += worker->errors;
    short_reads += worker->short_reads;
    corrupt += worker->corrupt;
  }
  if (latencies.empty() ||
      (config.seconds == 0 && latencies.size() != config.operations)) {
    throw std::runtime_error("missing completions");
  }
  std::sort(latencies.begin(), latencies.end());
  auto percentile = [&](double quantile) {
    return latencies[static_cast<size_t>(quantile * (latencies.size() - 1))];
  };
  size_t operations = latencies.size();
  double cpu_us =
      (cpu_seconds(cpu_after) - cpu_seconds(cpu_before)) * 1e6 / operations;
  const char *reactor =
#if defined(ASIO_HAS_IO_URING_AS_DEFAULT)
      "uring";
#else
      "epoll";
#endif
  std::printf(
      "%s,%s,%s,%u,%zu,%u,%d,%zu,%zu,%.6f,%.1f,%.3f,%.3f,%.3f,%.3f,%zu,%zu,%zu,"
      "%llu,%.4f,%.4f,%u,%d,%d,%d,%.3f,%zu,%zu,%d,%u,%d\n",
      reactor, backend.c_str(), config.sync_driver ? "sync" : "coroutine",
      depth, config.files, repeat, config.direct, config.read_size, operations,
      seconds, operations / seconds, percentile(0.5), percentile(0.99),
      percentile(0.999), cpu_us, errors, short_reads, corrupt,
      static_cast<unsigned long long>(after.max_inflight),
      double(after.enter_calls - before.enter_calls) / operations,
      double(after.wake_writes - before.wake_writes) / operations,
      rings.empty() ? 0 : rings.front()->setup_flags(), config.post_resume,
      config.poll, config.verify_data, config.seconds, data.blocks * block_size,
      rings.empty() ? size_t{1} : rings.size(), config.pin_owners,
      unsigned(ASIO_VERSION),
      coro_io::default_random_file_execution ==
          coro_io::execution_type::own_ring);
  std::fflush(stdout);
  for (const auto &ring : rings) {
    ring->stop();
    if (ring->statistics().outstanding != 0) {
      throw std::runtime_error("outstanding requests after benchmark");
    }
  }
  return errors == 0 && short_reads == 0 && corrupt == 0;
}

std::vector<std::string> split(const std::string &value) {
  std::vector<std::string> result;
  std::stringstream stream(value);
  std::string item;
  while (std::getline(stream, item, ',')) {
    result.push_back(item);
  }
  return result;
}

int main(int argc, char **argv) {
  try {
    Configuration config;
    for (int index = 1; index < argc; ++index) {
      std::string option = argv[index];
      if (option == "--help") {
        std::puts(
            "--operations N --file-mib N --read-size N --files N --rings N "
            "--repeats N\n"
            "--depths 1,8,32,128 --backends native,plain,single,defer\n"
            "--driver coroutine|sync --buffered --post-resume --poll\n"
            "--seconds N --ramp-seconds N --data-file PATH --prepare-file PATH "
            "--no-verify --pin-owners");
        return 0;
      }
      if (option == "--no-verify") {
        config.verify_data = false;
        continue;
      }
      if (option == "--pin-owners") {
        config.pin_owners = true;
        continue;
      }
      if (option == "--buffered") {
        config.direct = false;
        continue;
      }
      if (option == "--post-resume") {
        config.post_resume = true;
        continue;
      }
      if (option == "--poll") {
        config.poll = true;
        continue;
      }
      if (++index == argc) {
        throw std::invalid_argument("missing option value");
      }
      std::string value = argv[index];
      if (option == "--seconds")
        config.seconds = std::stod(value);
      else if (option == "--ramp-seconds")
        config.ramp_seconds = std::stod(value);
      else if (option == "--data-file")
        config.data_file = value;
      else if (option == "--prepare-file")
        config.prepare_file = value;
      else if (option == "--operations")
        config.operations = std::stoull(value);
      else if (option == "--file-mib")
        config.file_mib = std::stoull(value);
      else if (option == "--read-size")
        config.read_size = std::stoull(value);
      else if (option == "--files")
        config.files = std::stoull(value);
      else if (option == "--rings")
        config.rings = std::stoul(value);
      else if (option == "--repeats")
        config.repeats = std::stoul(value);
      else if (option == "--driver" &&
               (value == "sync" || value == "coroutine"))
        config.sync_driver = value == "sync";
      else if (option == "--backends")
        config.backends = split(value);
      else if (option == "--depths") {
        config.depths.clear();
        for (const auto &depth : split(value))
          config.depths.push_back(std::stoul(depth));
      }
      else
        throw std::invalid_argument("unknown option: " + option);
    }
    if (!config.operations || config.operations > 10000000 ||
        !config.file_mib || config.file_mib > 1024 || !config.files ||
        config.files > 256 || !config.rings || config.rings > config.files ||
        !config.repeats || config.repeats > 100 || !config.read_size ||
        config.read_size % block_size ||
        config.read_size > config.file_mib * 1024 * 1024 ||
        config.depths.empty() || config.backends.empty() ||
        !std::isfinite(config.seconds) || config.seconds < 0 ||
        config.seconds > 30 || !std::isfinite(config.ramp_seconds) ||
        config.ramp_seconds < 0 || config.ramp_seconds > 30 ||
        (!config.data_file.empty() && !config.prepare_file.empty())) {
      throw std::invalid_argument("invalid benchmark size");
    }
    for (auto depth : config.depths) {
      if (!depth || depth > 16384 || depth > config.operations ||
          (config.sync_driver && depth > 1024))
        throw std::invalid_argument("invalid queue depth");
    }
    for (const auto &backend : config.backends) {
      if (backend != "native" && backend != "plain" && backend != "single" &&
          backend != "defer")
        throw std::invalid_argument("invalid backend");
    }
    std::unique_ptr<file_io_test::TemporaryFile> generated;
    DataSet data;
    if (config.data_file.empty()) {
      generated = std::make_unique<file_io_test::TemporaryFile>(
          config.file_mib * 1024 * 1024 / block_size);
      data = {generated->path(), generated->blocks()};
      if (!config.prepare_file.empty()) {
        if (::link(data.path.c_str(), config.prepare_file.c_str()) < 0) {
          throw std::system_error(
              errno, std::system_category(),
              "publish data file (must not exist; same filesystem)");
        }
        std::printf("prepared %s (%zu bytes)\n", config.prepare_file.c_str(),
                    data.blocks * block_size);
        return 0;
      }
    }
    else {
      auto bytes = std::filesystem::file_size(config.data_file);
      if (!std::filesystem::is_regular_file(config.data_file) ||
          bytes % block_size || bytes < config.read_size) {
        throw std::invalid_argument("invalid existing data file");
      }
      data = {config.data_file, bytes / block_size};
    }
    std::puts(
        "reactor,backend,driver,depth,files,repeat,direct,read_size,operations,"
        "seconds,iops,p50_us,p99_us,p999_us,cpu_us_per_io,errors,short_reads,"
        "corrupt,max_read_sqes,enter_calls_per_io,wakes_per_io,setup_flags,"
        "post_resume,poll,verify_data,seconds_requested,file_bytes,rings,pin_"
        "owners,asio_version,default_own_ring");
    bool passed = true;
    for (auto depth : config.depths) {
      for (unsigned repeat = 0; repeat < config.repeats; ++repeat) {
        for (size_t index = 0; index < config.backends.size(); ++index) {
          const auto &backend =
              config.backends[(index + repeat) % config.backends.size()];
          if (backend == "native") {
            passed &= benchmark<coro_io::native_random_coro_file>(
                config, data, backend, depth, repeat);
          }
          else {
            passed &= benchmark<coro_io::own_ring_random_coro_file>(
                config, data, backend, depth, repeat);
          }
        }
      }
    }
    return passed ? 0 : 1;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "benchmark failed: %s\n", error.what());
    return 1;
  }
}
