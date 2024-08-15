#pragma once
#include <atomic>
#include <string>

#include "ylt/metric/counter.hpp"
#include "ylt/metric/gauge.hpp"
#include "ylt/metric/histogram.hpp"
#include "ylt/metric/metric.hpp"
#include "ylt/metric/metric_manager.hpp"
#include "ylt/metric/summary.hpp"
#include "ylt/metric/system_metric.hpp"

namespace cinatra {
struct cinatra_metric_conf {
  inline static std::string server_total_req = "server_total_req";
  inline static std::string server_failed_req = "server_failed_req";
  inline static std::string server_total_fd = "server_total_fd";
  inline static std::string server_total_recv_bytes = "server_total_recv_bytes";
  inline static std::string server_total_send_bytes = "server_total_send_bytes";
  inline static std::string server_req_latency = "server_req_latency";
  inline static std::string server_read_latency = "server_read_latency";
  inline static std::string server_total_thread_num = "server_total_thread_num";
  inline static bool enable_metric = false;

  inline static void server_total_req_inc() {
    if (!enable_metric) {
      return;
    }

    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::counter_t>(server_total_req);
    if (m == nullptr) {
      return;
    }
    m->inc();
  }

  inline static void server_failed_req_inc() {
    if (!enable_metric) {
      return;
    }
    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::counter_t>(server_failed_req);
    if (m == nullptr) {
      return;
    }
    m->inc();
  }

  inline static void server_total_fd_inc() {
    if (!enable_metric) {
      return;
    }
    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::gauge_t>(server_total_fd);
    if (m == nullptr) {
      return;
    }
    m->inc();
  }

  inline static void server_total_fd_dec() {
    if (!enable_metric) {
      return;
    }
    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::gauge_t>(server_total_fd);
    if (m == nullptr) {
      return;
    }
    m->dec();
  }

  inline static void server_total_recv_bytes_inc(double val) {
    if (!enable_metric) {
      return;
    }
    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::counter_t>(server_total_recv_bytes);
    if (m == nullptr) {
      return;
    }
    m->inc(val);
  }

  inline static void server_total_send_bytes_inc(double val) {
    if (!enable_metric) {
      return;
    }
    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::counter_t>(server_total_send_bytes);
    if (m == nullptr) {
      return;
    }
    m->inc(val);
  }

  inline static void server_req_latency_observe(double val) {
    if (!enable_metric) {
      return;
    }
    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::histogram_t>(server_req_latency);
    if (m == nullptr) {
      return;
    }
    m->observe(val);
  }

  inline static void server_read_latency_observe(double val) {
    if (!enable_metric) {
      return;
    }
    static auto m =
        ylt::metric::default_static_metric_manager::instance()
            .get_metric_static<ylt::metric::histogram_t>(server_read_latency);
    if (m == nullptr) {
      return;
    }
    m->observe(val);
  }
};
}  // namespace cinatra
