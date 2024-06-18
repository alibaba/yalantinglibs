#define DOCTEST_CONFIG_IMPLEMENT
#include <random>

#include "doctest.h"
#include "ylt/metric.hpp"

using namespace ylt;
using namespace ylt::metric;

TEST_CASE("test no lable") {
  {
    gauge_t g{"test_gauge", "help"};
    g.inc();
    g.inc();

    std::string str;
    g.serialize(str);
    CHECK(str.find("test_gauge 2") != std::string::npos);

    g.dec();
    CHECK(g.value() == 1);
    CHECK_THROWS_AS(g.dec({}, 1), std::invalid_argument);
    CHECK_THROWS_AS(g.inc({}, 1), std::invalid_argument);
    CHECK_THROWS_AS(g.update({}, 1), std::invalid_argument);

    counter_t c{"test_counter", "help"};
    c.inc();
    c.inc();
    std::string str1;
    c.serialize(str1);
    CHECK(str1.find("test_counter 2") != std::string::npos);
  }
  {
    counter_t c("get_count", "get counter");
    CHECK(c.metric_type() == MetricType::Counter);
    CHECK(c.labels_name().empty());
    c.inc();
    CHECK(c.value() == 1);
    c.inc();
    CHECK(c.value() == 2);
    c.inc(0);

    CHECK(c.value() == 2);

    CHECK_THROWS_AS(c.inc(-2), std::invalid_argument);
    CHECK_THROWS_AS(c.inc({}, 1), std::invalid_argument);
    CHECK_THROWS_AS(c.update({}, 1), std::invalid_argument);

    c.update(10);
    CHECK(c.value() == 10);

    c.update(0);
    CHECK(c.value() == 0);
  }
}

TEST_CASE("test with atomic") {
  counter_t c(
      "get_count", "get counter",
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}});
  std::vector<std::string> labels_value{"GET", "/"};
  c.inc(labels_value);
  c.inc(labels_value, 2);
  CHECK(c.value(labels_value) == 3);
  CHECK_THROWS_AS(c.inc({"GET", "/test"}), std::invalid_argument);
  CHECK_THROWS_AS(c.inc({"POST", "/"}), std::invalid_argument);
  c.update(labels_value, 10);
  CHECK(c.value(labels_value) == 10);

  gauge_t g(
      "get_qps", "get qps",
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}});
  g.inc(labels_value);
  g.inc(labels_value, 2);
  CHECK(g.value(labels_value) == 3);
  CHECK_THROWS_AS(g.inc({"GET", "/test"}), std::invalid_argument);
  CHECK_THROWS_AS(g.inc({"POST", "/"}), std::invalid_argument);
  g.dec(labels_value);
  g.dec(labels_value, 1);
  CHECK(g.value(labels_value) == 1);

  std::string str;
  c.serialize(str);
  std::cout << str;
  std::string str1;
  g.serialize(str1);
  std::cout << str1;
  CHECK(str.find("} 10") != std::string::npos);
  CHECK(str1.find("} 1") != std::string::npos);
}

TEST_CASE("test counter with dynamic labels value") {
  {
    auto c = std::make_shared<counter_t>(
        "get_count", "get counter", std::vector<std::string>{"method", "code"});
    CHECK(c->name() == "get_count");
    auto g = std::make_shared<gauge_t>(
        "get_count", "get counter", std::vector<std::string>{"method", "code"});
    CHECK(g->name() == "get_count");
    CHECK(g->metric_name() == "gauge");
  }

  {
    counter_t c("get_count", "get counter",
                std::vector<std::string>{"method", "code"});
    CHECK(c.labels_name() == std::vector<std::string>{"method", "code"});
    c.inc({"GET", "200"}, 1);
    auto values = c.value_map();
    CHECK(values[{"GET", "200"}] == 1);
    c.inc({"GET", "200"}, 2);
    values = c.value_map();
    CHECK(values[{"GET", "200"}] == 3);

    std::string str;
    c.serialize(str);
    std::cout << str;
    CHECK(str.find("# TYPE get_count counter") != std::string::npos);
    CHECK(str.find("get_count{method=\"GET\",code=\"200\"} 3") !=
          std::string::npos);

    CHECK_THROWS_AS(c.inc({"GET", "200", "/"}, 2), std::invalid_argument);

    c.update({"GET", "200"}, 20);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    values = c.value_map();
    CHECK(values[{"GET", "200"}] == 20);
  }
}

TEST_CASE("test gauge") {
  {
    gauge_t g("get_count", "get counter");
    CHECK(g.metric_type() == MetricType::Gauge);
    CHECK(g.labels_name().empty());
    g.inc();
    CHECK(g.value() == 1);
    g.inc();
    CHECK(g.value() == 2);
    g.inc(0);

#ifdef CINATRA_ENABLE_METRIC_JSON
    std::string str_json;
    g.serialize_to_json(str_json);
    std::cout << str_json << "\n";
    CHECK(str_json.find("\"value\":2") != std::string::npos);
#endif

    g.dec();
    CHECK(g.value() == 1);
    g.dec();
    CHECK(g.value() == 0);
  }

  {
    gauge_t g("get_count", "get counter", {"method", "code", "url"});
    CHECK(g.labels_name() == std::vector<std::string>{"method", "code", "url"});
    // method, status code, url
    g.inc({"GET", "200", "/"}, 1);
    auto values = g.value_map();
    CHECK(values[{"GET", "200", "/"}] == 1);
    g.inc({"GET", "200", "/"}, 2);
    values = g.value_map();
    CHECK(values[{"GET", "200", "/"}] == 3);

    g.inc({"POST", "200", "/"}, 4);

#ifdef CINATRA_ENABLE_METRIC_JSON
    std::string str_json;
    g.serialize_to_json(str_json);
    std::cout << str_json << "\n";
    CHECK(str_json.find("\"code\":\"200\"") != std::string::npos);
#endif

    std::string str;
    g.serialize(str);
    std::cout << str;
    CHECK(str.find("# TYPE get_count gauge") != std::string::npos);
    CHECK(str.find("get_count{method=\"GET\",code=\"200\",url=\"/\"} 3") !=
          std::string::npos);

    CHECK_THROWS_AS(g.dec({"GET", "200"}, 1), std::invalid_argument);

    g.dec({"GET", "200", "/"}, 1);
    values = g.value_map();
    CHECK(values[{"GET", "200", "/"}] == 2);
    g.dec({"GET", "200", "/"}, 2);
    values = g.value_map();
    CHECK(values[{"GET", "200", "/"}] == 0);
  }
}

TEST_CASE("test histogram") {
  histogram_t h("test", "help", {5.23, 10.54, 20.0, 50.0, 100.0});
  h.observe(23);
  auto counts = h.get_bucket_counts();
  CHECK(counts[3]->value() == 1);
  h.observe(42);
  CHECK(counts[3]->value() == 2);
  h.observe(60);
  CHECK(counts[4]->value() == 1);
  h.observe(120);
  CHECK(counts[5]->value() == 1);
  h.observe(1);
  CHECK(counts[0]->value() == 1);
  std::string str;
  h.serialize(str);
  std::cout << str << "\n";
  CHECK(str.find("test_count") != std::string::npos);
  CHECK(str.find("test_sum") != std::string::npos);
  CHECK(str.find("test_bucket{le=\"5.23") != std::string::npos);
  CHECK(str.find("test_bucket{le=\"+Inf\"}") != std::string::npos);

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string str_json;
  h.serialize_to_json(str_json);
  std::cout << str_json << "\n";
  CHECK(str_json.find("\"5.23\":1") != std::string::npos);
#endif
}

TEST_CASE("test summary") {
  summary_t summary{"test_summary",
                    "summary help",
                    {{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}}};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(1, 100);
  for (int i = 0; i < 50; i++) {
    summary.observe(distr(gen));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::string str;
  async_simple::coro::syncAwait(summary.serialize_async(str));
  std::cout << str;
  CHECK(async_simple::coro::syncAwait(summary.get_count()) == 50);
  CHECK(async_simple::coro::syncAwait(summary.get_sum()) > 0);
  CHECK(str.find("test_summary") != std::string::npos);
  CHECK(str.find("test_summary_count") != std::string::npos);
  CHECK(str.find("test_summary_sum") != std::string::npos);
  CHECK(str.find("test_summary{quantile=\"") != std::string::npos);

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string str_json;
  async_simple::coro::syncAwait(summary.serialize_to_json_async(str_json));
  std::cout << str_json << "\n";
  CHECK(str_json.find("\"0.9\":") != std::string::npos);
#endif
}

TEST_CASE("test register metric") {
  auto c = std::make_shared<counter_t>(std::string("get_count"),
                                       std::string("get counter"));
  default_metric_manager::register_metric_static(c);
  CHECK_FALSE(default_metric_manager::register_metric_static(c));

  auto g = std::make_shared<gauge_t>(std::string("get_guage_count"),
                                     std::string("get counter"));
  default_metric_manager::register_metric_static(g);

  auto map1 = default_metric_manager::metric_map_static();
  for (auto& [k, v] : map1) {
    bool r = k == "get_count" || k == "get_guage_count";
    break;
  }

  CHECK(default_metric_manager::metric_count_static() >= 2);
  CHECK(default_metric_manager::metric_keys_static().size() >= 2);

  c->inc();
  g->inc();

  auto map = default_metric_manager::metric_map_static();
  CHECK(map["get_count"]->as<counter_t>()->value() == 1);
  CHECK(map["get_guage_count"]->as<gauge_t>()->value() == 1);

  auto s = default_metric_manager::serialize_static();
  std::cout << s << "\n";
  CHECK(s.find("get_count 1") != std::string::npos);
  CHECK(s.find("get_guage_count 1") != std::string::npos);

  auto m = default_metric_manager::get_metric_static<counter_t>("get_count");
  CHECK(m->as<counter_t>()->value() == 1);

  auto m1 =
      default_metric_manager::get_metric_static<gauge_t>("get_guage_count");
  CHECK(m1->as<gauge_t>()->value() == 1);

  {
    // because the first regiter_metric is set no lock, so visit
    // default_metric_manager with lock will throw.
    auto c1 = std::make_shared<counter_t>(std::string(""), std::string(""));
    CHECK_THROWS_AS(default_metric_manager::register_metric_dynamic(c1),
                    std::invalid_argument);
    CHECK_THROWS_AS(default_metric_manager::metric_count_dynamic(),
                    std::invalid_argument);
    CHECK_THROWS_AS(default_metric_manager::metric_keys_dynamic(),
                    std::invalid_argument);
    CHECK_THROWS_AS(default_metric_manager::metric_map_dynamic(),
                    std::invalid_argument);
    CHECK_THROWS_AS(default_metric_manager::get_metric_dynamic<counter_t>(""),
                    std::invalid_argument);
  }
}

TEST_CASE("test remove metric and serialize metrics") {
  using metric_mgr = metric_manager_t<1>;
  metric_mgr::create_metric_dynamic<counter_t>("test_counter", "");
  metric_mgr::create_metric_dynamic<counter_t>("test_counter2", "");

  size_t count = metric_mgr::metric_count_dynamic();
  CHECK(count == 2);

  metric_mgr::remove_metric_dynamic("test_counter");
  count = metric_mgr::metric_count_dynamic();
  CHECK(count == 1);

  metric_mgr::remove_metric_dynamic("test_counter2");
  count = metric_mgr::metric_count_dynamic();
  CHECK(count == 0);

  CHECK_THROWS_AS(
      metric_mgr::create_metric_static<counter_t>("test_static_counter", ""),
      std::invalid_argument);

  using metric_mgr2 = metric_manager_t<2>;
  auto c =
      metric_mgr2::create_metric_static<counter_t>("test_static_counter", "");
  auto c2 =
      metric_mgr2::create_metric_static<counter_t>("test_static_counter2", "");
  c->inc();
  c2->inc();

#ifdef CINATRA_ENABLE_METRIC_JSON
  auto s = metric_mgr2::serialize_to_json_static();
  std::cout << s << "\n";
  auto s1 = metric_mgr2::serialize_to_json({c, c2});
  CHECK(s == s1);
#endif
  CHECK_THROWS_AS(metric_mgr2::metric_count_dynamic(), std::invalid_argument);
  count = metric_mgr2::metric_count_static();
  CHECK(count == 2);
  CHECK_THROWS_AS(metric_mgr2::remove_metric_dynamic("test_static_counter"),
                  std::invalid_argument);

  metric_mgr2::remove_metric_static("test_static_counter");
  count = metric_mgr2::metric_count_static();
  CHECK(count == 1);
}

TEST_CASE("test filter metrics static") {
  using metric_mgr = metric_manager_t<3>;
  auto c = metric_mgr::create_metric_static<counter_t>(
      "test_static_counter", "",
      std::map<std::string, std::string>{{"method", "GET"}});
  auto c2 = metric_mgr::create_metric_static<counter_t>(
      "test_static_counter2", "",
      std::map<std::string, std::string>{{"url", "/"}});
  c->inc({"GET"});
  c2->inc({"/"});

  metric_filter_options options;
  options.name_regex = ".*counter.*";
  {
    auto metrics = metric_mgr::filter_metrics_static(options);
    CHECK(metrics.size() == 2);

    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_static_counter") != std::string::npos);
    std::cout << s << "\n";
  }

  options.label_regex = ".*ur.*";
  {
    auto metrics = metric_mgr::filter_metrics_static(options);
    CHECK(metrics.size() == 1);
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_static_counter2") != std::string::npos);
    std::cout << s << "\n";
  }

  options.name_regex = "no_such_name";
  {
    auto metrics = metric_mgr::filter_metrics_static(options);
    CHECK(metrics.empty());
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.empty());
  }

  options = {};
  options.label_regex = "no_such_label";
  {
    auto metrics = metric_mgr::filter_metrics_static(options);
    CHECK(metrics.empty());
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.empty());
  }

  // don't filter
  options = {};
  {
    auto metrics = metric_mgr::filter_metrics_static(options);
    CHECK(metrics.size() == 2);
  }

  // black
  options.label_regex = ".*ur.*";
  options.is_white = false;
  {
    auto metrics = metric_mgr::filter_metrics_static(options);
    CHECK(metrics.size() == 1);
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_static_counter") != std::string::npos);
    CHECK(s.find("test_static_counter2") == std::string::npos);
  }

  options = {};
  options.label_regex = ".*ur.*";
  options.is_white = false;
  {
    auto metrics = metric_mgr::filter_metrics_static(options);
    CHECK(metrics.size() == 1);
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_static_counter") != std::string::npos);
    CHECK(s.find("method") != std::string::npos);
    CHECK(s.find("test_static_counter2") == std::string::npos);
    CHECK(s.find("url") == std::string::npos);
  }
}

TEST_CASE("test filter metrics dynamic") {
  using metric_mgr = metric_manager_t<4>;
  auto c = metric_mgr::create_metric_dynamic<counter_t>(
      "test_dynamic_counter", "", std::vector<std::string>{{"method"}});
  auto c2 = metric_mgr::create_metric_dynamic<counter_t>(
      "test_dynamic_counter2", "", std::vector<std::string>{{"url"}});
  c->inc({"GET"});
  c->inc({"POST"});
  c2->inc({"/"});
  c2->inc({"/test"});

  metric_filter_options options;
  options.name_regex = ".*counter.*";
  {
    auto metrics = metric_mgr::filter_metrics_dynamic(options);
    CHECK(metrics.size() == 2);

    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_dynamic_counter") != std::string::npos);
    std::cout << s << "\n";
  }

  options.label_regex = ".*ur.*";
  {
    auto metrics = metric_mgr::filter_metrics_dynamic(options);
    CHECK(metrics.size() == 1);
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_dynamic_counter2") != std::string::npos);
    std::cout << s << "\n";
  }

  options.name_regex = "no_such_name";
  {
    auto metrics = metric_mgr::filter_metrics_dynamic(options);
    CHECK(metrics.empty());
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.empty());
  }

  options = {};
  options.label_regex = "no_such_label";
  {
    auto metrics = metric_mgr::filter_metrics_dynamic(options);
    CHECK(metrics.empty());
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.empty());
  }

  // don't filter
  options = {};
  {
    auto metrics = metric_mgr::filter_metrics_dynamic(options);
    CHECK(metrics.size() == 2);
  }

  // black
  options.label_regex = ".*ur.*";
  options.is_white = false;
  {
    auto metrics = metric_mgr::filter_metrics_dynamic(options);
    CHECK(metrics.size() == 1);
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_dynamic_counter") != std::string::npos);
    CHECK(s.find("test_dynamic_counter2") == std::string::npos);
  }

  options = {};
  options.label_regex = ".*ur.*";
  options.is_white = false;
  {
    auto metrics = metric_mgr::filter_metrics_dynamic(options);
    CHECK(metrics.size() == 1);
    auto s = metric_mgr::serialize(metrics);
    CHECK(s.find("test_dynamic_counter") != std::string::npos);
    CHECK(s.find("method") != std::string::npos);
    CHECK(s.find("test_dynamic_counter2") == std::string::npos);
    CHECK(s.find("url") == std::string::npos);
  }
}

TEST_CASE("test get metric by static labels and label") {
  using metric_mgr = metric_manager_t<9>;
  metric_mgr::create_metric_static<counter_t>(
      "http_req_test", "",
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}});
  metric_mgr::create_metric_static<gauge_t>(
      "http_req_test1", "",
      std::map<std::string, std::string>{{"method", "POST"}, {"url", "/"}});
  metric_mgr::create_metric_static<counter_t>(
      "http_req_test2", "",
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/test"}});

  auto v = metric_mgr::get_metric_by_labels_static(
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/test"}});
  CHECK(v[0]->name() == "http_req_test2");

  v = metric_mgr::get_metric_by_labels_static(
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}});
  CHECK(v[0]->name() == "http_req_test");

  auto h1 = metric_mgr::create_metric_static<histogram_t>(
      "http_req_static_hist", "help",
      std::vector<double>{5.23, 10.54, 20.0, 50.0, 100.0},
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}});

  h1->observe({"GET", "/"}, 23);

  auto s1 = metric_mgr::create_metric_static<summary_t>(
      "http_req_static_summary", "help",
      summary_t::Quantiles{
          {0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}},
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}});
  s1->observe({"GET", "/"}, 23);

  auto vec = metric_mgr::get_metric_by_label_static({"method", "GET"});
  CHECK(vec.size() == 4);

  vec = metric_mgr::get_metric_by_label_static({"url", "/"});
  CHECK(vec.size() == 4);

  vec = metric_mgr::get_metric_by_label_static({"url", "/test"});
  CHECK(vec.size() == 1);

  vec = metric_mgr::get_metric_by_label_static({"method", "POST"});
  CHECK(vec.size() == 1);

  vec = metric_mgr::get_metric_by_labels_static(
      std::map<std::string, std::string>{{"method", "HEAD"}, {"url", "/test"}});
  CHECK(vec.empty());

  vec = metric_mgr::get_metric_by_labels_static(
      std::map<std::string, std::string>{{"method", "GET"}});
  CHECK(vec.empty());

  vec = metric_mgr::get_metric_by_label_static({"url", "/index"});
  CHECK(vec.empty());
}

TEST_CASE("test get metric by dynamic labels") {
  using metric_mgr = metric_manager_t<10>;
  auto c = metric_mgr::create_metric_dynamic<counter_t>(
      "http_req_static", "", std::vector<std::string>{"method", "code"});

  auto c1 = metric_mgr::create_metric_dynamic<counter_t>(
      "http_req_static1", "", std::vector<std::string>{"method", "code"});

  auto c2 = metric_mgr::create_metric_dynamic<counter_t>(
      "http_req_static2", "", std::vector<std::string>{"method", "code"});

  auto c3 = metric_mgr::create_metric_dynamic<counter_t>(
      "http_req_static3", "", std::vector<std::string>{"method", "code"});

  c->inc({"POST", "200"});
  c1->inc({"GET", "200"});
  c2->inc({"POST", "301"});
  c3->inc({"POST", "400"});

  auto c4 = metric_mgr::create_metric_dynamic<counter_t>(
      "http_req_static4", "", std::vector<std::string>{"host", "url"});

  auto c5 = metric_mgr::create_metric_dynamic<counter_t>(
      "http_req_static5", "", std::vector<std::string>{"host", "url"});

  c4->inc({"shanghai", "/"});
  c5->inc({"shanghai", "/test"});

  auto vec = metric_mgr::get_metric_by_labels_dynamic({{"method", "POST"}});
  CHECK(vec.size() == 3);

  vec = metric_mgr::get_metric_by_labels_dynamic({{"method", "GET"}});
  CHECK(vec.size() == 1);

  vec = metric_mgr::get_metric_by_labels_dynamic({{"host", "shanghai"}});
  CHECK(vec.size() == 2);

  vec = metric_mgr::get_metric_by_labels_dynamic({{"url", "/"}});
  CHECK(vec.size() == 1);

  vec = metric_mgr::get_metric_by_labels_dynamic({{"url", "/test"}});
  CHECK(vec.size() == 1);

  vec = metric_mgr::get_metric_by_labels_dynamic({{"url", "/none"}});
  CHECK(vec.size() == 0);

  vec = metric_mgr::get_metric_by_labels_dynamic({{"method", "HEAD"}});
  CHECK(vec.size() == 0);

  auto h1 = metric_mgr::create_metric_dynamic<histogram_t>(
      "http_req_static_hist", "help",
      std::vector<double>{5.23, 10.54, 20.0, 50.0, 100.0},
      std::vector<std::string>{"method", "url"});

  h1->observe({"GET", "/"}, 23);

  auto s1 = metric_mgr::create_metric_dynamic<summary_t>(
      "http_req_static_summary", "help",
      summary_t::Quantiles{
          {0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}},
      std::vector<std::string>{"method", "url"});
  s1->observe({"GET", "/"}, 23);

  vec = metric_mgr::get_metric_by_labels_dynamic({{"method", "GET"}});
  CHECK(vec.size() >= 2);

  auto str = metric_mgr::serialize(vec);
  std::cout << str;

#ifdef CINATRA_ENABLE_METRIC_JSON
  auto json_str = metric_mgr::serialize_to_json(vec);
  std::cout << json_str << "\n";
#endif
}

TEST_CASE("test histogram serialize with dynamic labels") {
  histogram_t h("test", "help", {5.23, 10.54, 20.0, 50.0, 100.0},
                std::vector<std::string>{"method", "url"});
  h.observe({"GET", "/"}, 23);
  auto counts = h.get_bucket_counts();
  CHECK(counts[3]->value({"GET", "/"}) == 1);
  h.observe({"GET", "/"}, 42);
  CHECK(counts[3]->value({"GET", "/"}) == 2);
  h.observe({"GET", "/"}, 60);
  CHECK(counts[4]->value({"GET", "/"}) == 1);
  h.observe({"GET", "/"}, 120);
  CHECK(counts[5]->value({"GET", "/"}) == 1);
  h.observe({"GET", "/"}, 1);
  CHECK(counts[0]->value({"GET", "/"}) == 1);

  h.observe({"POST", "/"}, 23);
  CHECK(counts[3]->value({"POST", "/"}) == 1);
  h.observe({"POST", "/"}, 42);
  CHECK(counts[3]->value({"POST", "/"}) == 2);
  h.observe({"POST", "/"}, 60);
  CHECK(counts[4]->value({"POST", "/"}) == 1);
  h.observe({"POST", "/"}, 120);
  CHECK(counts[5]->value({"POST", "/"}) == 1);
  h.observe({"POST", "/"}, 1);
  CHECK(counts[0]->value({"POST", "/"}) == 1);

  std::string str;
  h.serialize(str);
  std::cout << str;

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string str_json;
  h.serialize_to_json(str_json);
  std::cout << str_json << "\n";
#endif
}

TEST_CASE("test histogram serialize with static labels") {
  histogram_t h(
      "test", "help", {5.23, 10.54, 20.0, 50.0, 100.0},
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}});
  h.observe({"GET", "/"}, 23);
  auto counts = h.get_bucket_counts();
  CHECK(counts[3]->value({"GET", "/"}) == 1);
  h.observe({"GET", "/"}, 42);
  CHECK(counts[3]->value({"GET", "/"}) == 2);
  h.observe({"GET", "/"}, 60);
  CHECK(counts[4]->value({"GET", "/"}) == 1);
  h.observe({"GET", "/"}, 120);
  CHECK(counts[5]->value({"GET", "/"}) == 1);
  h.observe({"GET", "/"}, 1);
  CHECK(counts[0]->value({"GET", "/"}) == 1);

  std::string str;
  h.serialize(str);
  std::cout << str;

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string str_json;
  h.serialize_to_json(str_json);
  std::cout << str_json << "\n";
#endif
}

TEST_CASE("test summary with dynamic labels") {
  summary_t summary{"test_summary",
                    "summary help",
                    {{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}},
                    std::vector<std::string>{"method", "url"}};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(1, 100);
  for (int i = 0; i < 50; i++) {
    summary.observe({"GET", "/"}, distr(gen));
    summary.observe({"POST", "/test"}, distr(gen));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  double sum;
  uint64_t count;
  auto rates = async_simple::coro::syncAwait(
      summary.get_rates({"GET", "/"}, sum, count));
  std::cout << rates.size() << "\n";

  std::string str;
  async_simple::coro::syncAwait(summary.serialize_async(str));
  std::cout << str;

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string json_str;
  async_simple::coro::syncAwait(summary.serialize_to_json_async(json_str));
  std::cout << json_str << "\n";
#endif
}

TEST_CASE("test summary with static labels") {
  summary_t summary{
      "test_summary",
      "summary help",
      {{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}},
      std::map<std::string, std::string>{{"method", "GET"}, {"url", "/"}}};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(1, 100);
  for (int i = 0; i < 50; i++) {
    summary.observe({"GET", "/"}, distr(gen));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  CHECK_THROWS_AS(summary.observe({"POST", "/"}, 1), std::invalid_argument);

  double sum;
  uint64_t count;
  auto rates = async_simple::coro::syncAwait(
      summary.get_rates({"GET", "/"}, sum, count));
  std::cout << rates.size() << "\n";

  std::string str;
  async_simple::coro::syncAwait(summary.serialize_async(str));
  std::cout << str;

#ifdef CINATRA_ENABLE_METRIC_JSON
  std::string json_str;
  async_simple::coro::syncAwait(summary.serialize_to_json_async(json_str));
  std::cout << json_str << "\n";
#endif
}

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP