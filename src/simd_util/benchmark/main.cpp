#include <ylt/simd_util/simd_str_split.h>

#include <time.h>                            // timespec, clock_gettime
#include <sys/time.h>                        // timeval, gettimeofday
#include <stdint.h>                          // int64_t, uint64_t

#include <iostream>
#include <chrono>
#include <thread>

#include <random>
#include <benchmark/benchmark.h>


inline std::vector<std::string_view> normal_str_split(std::string_view s, const char delimiter) {
    size_t start = 0;
    size_t end = s.find_first_of(delimiter);

    std::vector<std::string_view> output;

    while (end <= std::string_view::npos) {
        output.emplace_back(s.substr(start, end - start));

        if (end == std::string_view::npos)
            break;

        start = end + 1;
        end = s.find_first_of(delimiter, start);
    }

    return output;
}

inline int64_t gettimeofday_us() {
    timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000L + now.tv_usec;
}

std::string generate_test_string(size_t length, char delimiter, size_t avg_segment_len) {
    std::string s;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> char_dist('a', 'z');
    std::uniform_int_distribution<size_t> seg_dist(1, 2 * avg_segment_len);

    while (s.size() < length) {
        size_t seg_len = seg_dist(gen);
        for (size_t i = 0; i < seg_len && s.size() < length; ++i) {
            s += static_cast<char>(char_dist(gen));
        }
        if (s.size() < length) s += delimiter;
    }
    return s;
}

// Benchmark runner
template<typename Func>
void BM_Split(benchmark::State& state, Func split_func, bool return_string_view) {
    const size_t str_len = state.range(0);
    const char delimiter = ',';
    const size_t avg_segment_len = 10;
    auto test_str = generate_test_string(str_len, delimiter, avg_segment_len);

    for (auto _ : state) {
        if (return_string_view) {
            auto result = split_func(test_str, delimiter);
            benchmark::DoNotOptimize(result);
        } else {
            auto result = split_func(test_str, delimiter);
            benchmark::DoNotOptimize(result);
        }
    }
}


BENCHMARK_CAPTURE(BM_Split, simd_str_split_sv, ylt::split_sv, true)
    ->Args({100})->Args({1000})->Args({10000});

BENCHMARK_CAPTURE(BM_Split, naive_str_split_sv, normal_str_split, true)
    ->Args({100})->Args({1000})->Args({10000});

BENCHMARK_MAIN();