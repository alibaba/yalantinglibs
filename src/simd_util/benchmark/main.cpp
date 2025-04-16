#include <ylt/simd_util/simd_str_split.h>

#include <time.h>                            // timespec, clock_gettime
#include <sys/time.h>                        // timeval, gettimeofday
#include <stdint.h>                          // int64_t, uint64_t

#include <iostream>

inline int64_t gettimeofday_us() {
    timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000L + now.tv_usec;
}

inline std::vector<std::string_view> normal_sv_split(std::string_view s,
    const char delim) {
    size_t start = 0;
    size_t end = s.find_first_of(delim);

    std::vector<std::string_view> output;

    while (end <= std::string_view::npos) {
        output.emplace_back(s.substr(start, end - start));

        if (end == std::string_view::npos)
        break;

        start = end + 1;
        end = s.find_first_of(delim, start);
    }

    return output;
}

#define REPEATED_NUMBER 10

void __attribute__((noinline)) perf_normal_split() {
    std::string_view sv_tmp = "hello world\t127.0.0.1\t1024\twww.yalantinglibs.com";
    int64_t begin = gettimeofday_us();
    for (size_t i = 0; i < REPEATED_NUMBER; ++i) {
        normal_sv_split(sv_tmp, '\t');
    }

    int64_t end = gettimeofday_us();

    std::cout << "normal split total use: " << (end - begin) * 1.0 << std::endl;
}

void __attribute__((noinline)) perf_simd_split() {
    std::string_view sv_tmp = "hello world\t127.0.0.1\t1024\twww.yalantinglibs.com";
    int64_t begin = gettimeofday_us();
    for (size_t i = 0; i < REPEATED_NUMBER; ++i) {
        ylt::split_sv(sv_tmp, '\t');
    }

    int64_t end = gettimeofday_us();

    std::cout << "simd split total use: " << (end - begin) * 1.0 << std::endl;
}

int main()
{
    perf_normal_split();
    perf_simd_split();
}