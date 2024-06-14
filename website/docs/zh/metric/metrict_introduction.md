# metric 介绍
metric 用于统计应用程序的各种指标，这些指标被用于系统见识和警报，常见的指标类型有四种：Counter、Gauge、Histogram和Summary，这些指标遵循[Prometheus](https://hulining.gitbook.io/prometheus/introduction)的数据格式。

## Counter 计数器类型
Counter是一个累计类型的数据指标，它代表单调递增的计数器，其值只能在重新启动时增加或重置为 0。例如，您可以使用计数器来表示已响应的请求数，已完成或出错的任务数。

不要使用计数器来显示可以减小的值。例如，请不要使用计数器表示当前正在运行的进程数；使用 gauge 代替。

## Gauge 数据轨迹类型
Gauge 是可以任意上下波动数值的指标类型。

Gauge 通常用于测量值，例如温度或当前的内存使用量，还可用于可能上下波动的"计数"，例如请求并发数。

如：

```cpp
# HELP node_cpu Seconds the cpus spent in each mode.
# TYPE node_cpu counter
node_cpu{cpu="cpu0",mode="idle"} 362812.7890625
# HELP node_load1 1m load average.
# TYPE node_load1 gauge
node_load1 3.0703125
```

## Histogram 直方图类型
Histogram 对观测值(通常是请求持续时间或响应大小之类的数据)进行采样，并将其计数在可配置的数值区间中。它也提供了所有数据的总和。

基本数据指标名称为<basename>的直方图类型数据指标，在数据采集期间会显示多个时间序列：

数值区间的累计计数器，显示为<basename>_bucket{le="<数值区间的上边界>"}

所有观测值的总和，显示为<basename>_sum

统计到的事件计数，显示为<basename>_count(与上述<basename>_bucket{le="+Inf"}相同)

如:

```cpp
# A histogram, which has a pretty complex representation in the text format:
# HELP http_request_duration_seconds A histogram of the request duration.
# TYPE http_request_duration_seconds histogram
http_request_duration_seconds_bucket{le="0.05"} 24054
http_request_duration_seconds_bucket{le="0.1"} 33444
http_request_duration_seconds_bucket{le="0.2"} 100392
http_request_duration_seconds_bucket{le="+Inf"} 144320
http_request_duration_seconds_sum 53423
http_request_duration_seconds_count 144320
```

## Summary 汇总类型
类似于 histogram，summary 会采样观察结果(通常是请求持续时间和响应大小之类的数据)。它不仅提供了观测值的总数和所有观测值的总和，还可以计算滑动时间窗口内的可配置分位数。

基本数据指标名称为<basename>的 summary 类型数据指标，在数据采集期间会显示多个时间序列：

流观察到的事件的 φ-quantiles(0≤φ≤1)，显示为<basename>{quantile="<φ>"}

所有观测值的总和，显示为<basename>_sum

观察到的事件计数，显示为<basename>_count

如：

```cpp
# HELP prometheus_tsdb_wal_fsync_duration_seconds Duration of WAL fsync.
# TYPE prometheus_tsdb_wal_fsync_duration_seconds summary
prometheus_tsdb_wal_fsync_duration_seconds{quantile="0.5"} 0.012352463
prometheus_tsdb_wal_fsync_duration_seconds{quantile="0.9"} 0.014458005
prometheus_tsdb_wal_fsync_duration_seconds{quantile="0.99"} 0.017316173
prometheus_tsdb_wal_fsync_duration_seconds_sum 2.888716127000002
prometheus_tsdb_wal_fsync_duration_seconds_count 216
```

# 概述
metric 包括4种指标类型：
- couter：只会增加的指标；
- gauge：可以增加或减少的指标，它派生于counter；
- histogram：直方图，初始化的时候需要设置桶(bucket)；
- summary：分位数指标，初始化的时候需要设置桶和误差；

# label

label：标签，可选，指标可以没有标签。标签是指一个键值对，标签的键需要在创建指标的时候设置，是静态不可变的。

标签的值可以在创建指标的时候设置，这样的label则被称为静态的label。

标签的值在运行期动态创建，则label被称为动态的label。

动态label的例子：

```cpp
{"method", "url"}
```
这个label只有键没有值，所以这个label是动态的label。后续动态生成label对应的值时，这样做：
```cpp
{"GET", "/"}
{"POST", "/test"}
```
使用的时候填动态的方法名和url就行了：

```cpp
some_counter.inc({std::string(req.method()), req.url()}, 1);
```
如果传入的标签值数量和创建时的label 键的数量不匹配时则会抛异常。

静态label的例子：

```cpp
{{"method", "GET"}, {"url", "/"}}
```
这个label的键值都确定了，是静态的，后面使用的时候需要显式调用静态的标签值使用:

```cpp
some_counter.inc({"GET", "/"}, 1);
```

无标签：创建指标的时候不设置标签，内部只有一个计数。

# counter和gauge的使用

## 创建没有标签的指标

```cpp
    gauge_t g{"test_gauge", "help"};
    g.inc();
    g.inc(1);

    std::string str;
    g.serialize(str);
    CHECK(str.find("test_gauge 2") != std::string::npos);

    g.dec(1);
    CHECK(g.value() == 1);
    g.update(1);

    CHECK_THROWS_AS(g.dec({"test"}, 1), std::invalid_argument);
    CHECK_THROWS_AS(g.inc({"test"}, 1), std::invalid_argument);
    CHECK_THROWS_AS(g.update({"test"}, 1), std::invalid_argument);

    counter_t c{"test_counter", "help"};
    c.inc();
    c.inc(1);
    std::string str1;
    c.serialize(str1);
    CHECK(str1.find("test_counter 2") != std::string::npos);
```
## counter/gauge指标的api

构造函数:

```cpp
// 无标签，调用inc时不带标签，如c.inc()，调用此函数则metric 为静态标签的metric
// name: 指标对象的名称，注册到指标管理器时会使用这个名称
// help: 指标对象的帮助信息
counter_t(std::string name, std::string help);

// labels: 静态标签，构造时需要将标签键值都填完整，如：{{"method", "GET"}, {"url", "/"}}
// 调用此函数则metric 为静态标签的metric
// 调用inc时必须带静态标签的值，如：c.inc({"GET", "/"}, 1);
counter_t(std::string name, std::string help,
            std::map<std::string, std::string> labels);

// labels_name: 动态标签的键名称，因为标签的值是动态的，而键的名称是固定的，所以这里只需要填键名称，如: {"method", "url"}
// 调用时inc时必须带动态标签的值，如：c.inc({method, url}, 1);
// 调用此函数则metric 为动态标签的metric
counter_t(std::string name, std::string help,
            std::vector<std::string> labels_name);
```

基本函数：

```cpp
// 获取无标签指标的计数，
double value();

// 根据标签获取指标的计数，参数为动态或者静态标签的值
double value(const std::vector<std::string> &labels_value);

// 无标签指标增加计数，counter的计数只能增加不能减少，如果填入的时负数时会抛异常；如果需要减少计数的指标则应该使用gauge；
void inc(double val = 1);

// 根据标签增加计数，如果创建的指标是静态标签值且和传入的标签值不匹配时会抛异常；如果标签的值的数量和构造指标时传入的标签数量不相等时会抛异常。
void inc(const std::vector<std::string> &labels_value, double value = 1);

// 序列化，将指标序列化成prometheus 格式的字符串
void serialize(std::string &str);

// 返回带标签的指标内部的计数map，map的key是标签的值，值是对应计数，如：{{{"GET", "/"}, 100}, {{"POST", "/test"}, 20}}
std::map<std::vector<std::string>, double,
           std::less<std::vector<std::string>>>
  value_map();
```

注意：如果使用动态标签的时候要注意这个动态的标签值是不是无限多的，如果是无限多的话，那么内部的map也会无限增长，应该避免这种情况，动态的标签也应该是有限的才对。

gauge 派生于counter，相比counter多了一个减少计数的api

```cpp
// 无标签指标减少计数
void dec(double value = 1);

// 根据标签减少计数，如果创建的指标是静态标签值且和传入的标签值不匹配时会抛异常；如果标签的值的数量和构造指标时传入的标签数量不相等时会抛异常。
void dec(const std::vector<std::string>& labels_value, double value = 1);
```

# 基类公共函数
所有指标都派生于metric_t 基类，提供了一些公共方法，如获取指标的名称，指标的类型，标签的键名称等等。

```cpp
class metric_t {
 public:
  // 获取指标对象的名称
  std::string_view name();

  // 获取指标对象的help 信息
  std::string_view help();

  // 指标类型
  enum class MetricType {
    Counter,
    Gauge,
    Histogram,
    Summary,
    Nil,
  };

  // 获取指标类型
  MetricType metric_type();

  // 获取指标类型的名称，比如counter, gauge, histogram和summary
  std::string_view metric_name();

  // 获取标签的键，如{"method", "url"}
  const std::vector<std::string>& labels_name();

  // 获取静态标签，如{{"method", "GET"}, {"code", "200"}}
  const std::map<std::string, std::string>& get_static_labels();

  // 序列化，调用派生类实现序列化
  virtual void serialize(std::string& str);

  // 给summary专用的api，序列化，调用派生类实现序列化
  virtual async_simple::coro::Lazy<void> serialize_async(std::string& out);

  // 序列化到json
  void serialize_to_json(std::string& str);

  // 将基类指针向下转换到派生类指针，如:
  // std::shared_ptr<metric_t> c = std::make_shared<counter_t>("test", "test");
  // counter_t* t = c->as<counter_t*>();
  // t->value();
  template <typename T>
  T* as();
};
```

# 指标管理器
如果希望集中管理多个指标时，则需要将指标注册到指标管理器，后面则可以多态调用管理器中的多个指标将各自的计数输出出来。

**推荐在一开始就创建所有的指标并注册到管理器**，后面就可以无锁方式根据指标对象的名称来获取指标对象了。

```cpp
auto c = std::make_shared<counter_t>("qps_count", "qps help");
auto g = std::make_shared<gauge_t>("fd_count", "fd count help");
default_metric_manager::register_metric_static(c);
default_metric_manager::register_metric_static(g);

c->inc();
g->inc();

auto m = default_metric_manager::get_metric_static("qps_count");
CHECK(m->as<counter_t>()->value() == 1);

auto m1 = default_metric_manager::get_metric_static("fd_count");
CHECK(m1->as<gauge_t>()->value() == 1);
```

如果希望动态注册的到管理器则应该调用register_metric_dynamic接口，后面根据名称获取指标对象时则调用get_metric_dynamic接口，dynamic接口内部会加锁。

```cpp
auto c = std::make_shared<counter_t>("qps_count", "qps help");
auto g = std::make_shared<gauge_t>("fd_count", "fd count help");
default_metric_manager::register_metric_dynamic(c);
default_metric_manager::register_metric_dynamic(g);

c->inc();
g->inc();

auto m = default_metric_manager::get_metric_dynamic("qps_count");
CHECK(m->as<counter_t>()->value() == 1);

auto m1 = default_metric_manager::get_metric_dynamic("fd_count");
CHECK(m1->as<gauge_t>()->value() == 1);
```
注意：一旦注册时使用了static或者dynamic，那么后面调用default_metric_manager时则应该使用相同后缀的接口，比如注册时使用了get_metric_static，那么后面调用根据名称获取指标对象的方法必须是get_metric_static，否则会抛异常。同样，如果注册使用register_metric_dynamic，则后面应该get_metric_dynamic，否则会抛异常。

指标管理器的api
```cpp
template <size_t ID = 0>
struct metric_manager_t {
  // 创建并注册指标，返回注册的指标对象
  template <typename T, typename... Args>
  static std::shared_ptr<T> create_metric_static(const std::string& name,
                                                 const std::string& help,
                                                 Args&&... args);
  template <typename T, typename... Args>
  static std::shared_ptr<T> create_metric_dynamic(const std::string& name,
                                                 const std::string& help,
                                                 Args&&... args)
  // 注册metric
  static bool register_metric_static(std::shared_ptr<metric_t> metric);
  static bool register_metric_dynamic(std::shared_ptr<metric_t> metric);

  // 根据metric名称删除metric
  static bool remove_metric_static(const std::string& name);  
  static bool remove_metric_dynamic(const std::string& name);

  // 获取注册的所有指标对象
  static std::map<std::string, std::shared_ptr<metric_t>> metric_map_static();
  static std::map<std::string, std::shared_ptr<metric_t>> metric_map_dynamic();

  // 获取注册的指标对象的总数
  static size_t metric_count_static();
  static size_t metric_count_dynamic();

  // 获取注册的指标对象的名称
  static std::vector<std::string> metric_keys_static();
  static std::vector<std::string> metric_keys_dynamic();

  // 根据名称获取指标对象，T为具体指标的类型，如 get_metric_static<gauge_t>();
  // 如果找不到则返回nullptr
  template <typename T>
  static T* get_metric_static(const std::string& name);
  template <typename T>
  static T* get_metric_static(const std::string& name);

  static std::shared_ptr<metric_t> get_metric_static(const std::string& name);
  static std::shared_ptr<metric_t> get_metric_dynamic(const std::string& name);

  // 根据静态标签获取所有的指标, 如{{"method", "GET"}, {"url", "/"}}
  static std::vector<std::shared_ptr<metric_t>> get_metric_by_labels_static(
      const std::map<std::string, std::string>& labels);

  // 根据标签值获取所有的静态标签的指标, 如{"method", "GET"}
  static std::vector<std::shared_ptr<metric_t>> get_metric_by_label_static(
      const std::pair<std::string, std::string>& label);

  // 根据标签值获取所有动态标签的指标, 如{"method", "GET"}
  static std::vector<std::shared_ptr<metric_t>> get_metric_by_labels_dynamic(
      const std::map<std::string, std::string>& labels);
  
  // 序列化
  static async_simple::coro::Lazy<std::string> serialize_static();
  static async_simple::coro::Lazy<std::string> serialize_dynamic();

  // 序列化静态标签的指标到json
  static std::string serialize_to_json_static();
  // 序列化动态标签的指标到json
  static std::string serialize_to_json_dynamic();
  // 序列化metric集合到json
  static std::string serialize_to_json(
      const std::vector<std::shared_ptr<metric_t>>& metrics);

  // 过滤配置选项，如果name_regex和label_regex都设置了，则会检查这两个条件，如果只设置了一个则只检查设置过的条件
  struct metric_filter_options {
    std::optional<std::regex> name_regex{}; // metric 名称的过滤正则表达式
    std::optional<std::regex> label_regex{};// metric label名称的过滤正则表达式
    bool is_white = true; //true: 白名单，包括语义；false: 黑名单，排除语义
  };

  // 过滤静态标签的指标
  static std::vector<std::shared_ptr<metric_t>> filter_metrics_static(
      const metric_filter_options& options);
  // 过滤动态标签的指标
  static std::vector<std::shared_ptr<metric_t>> filter_metrics_dynamic(
      const metric_filter_options& options);  
};
using default_metric_manager = metric_manager_t<0>;
```
metric_manager_t默认为default_metric_manager，如果希望有多个metric manager，用户可以自定义新的metric manager，如：

```cpp
constexpr size_t metric_id = 100;
using my_metric_manager = metric_manager_t<metric_id>;
```

# histogram

## api
```cpp
//
// name: 对象名称，help：帮助信息
// buckets：桶，如 {1, 3, 7, 11, 23}，后面设置的值会自动落到某个桶中并增加该桶的计数；
// 内部还有一个+Inf 默认的桶，当输入的数据不在前面设置这些桶中，则会落到+Inf 默认桶中。
// 实际上桶的总数为 buckets.size() + 1
// 每个bucket 实际上对应了一个counter指标
// 调用此函数，则metric为静态metric指标
histogram_t(std::string name, std::string help, std::vector<double> buckets);

// labels_value: 标签key，后面可以使用动态标签值去observe，调用此函数则metric为动态metric 指标
histogram_t(std::string name, std::string help, std::vector<double> buckets,
            std::vector<std::string> labels_name);

// labels: 静态标签，调用此函数则metric为静态metric指标
histogram_t(std::string name, std::string help, std::vector<double> buckets,
            std::map<std::string, std::string> labels);

// 往histogram_t 中插入数据，内部会自动增加对应桶的计数
void observe(double value);

// 根据标签值插入数据，可以是动态标签值也可以是静态标签值。如果是静态标签，会做额外的检车，检查传入的labels_value是否和注册时的静态标签值是否相同，不相同会抛异常；
void observe(const std::vector<std::string> &labels_value, double value);

// 获取所有桶对应的counter指标对象
std::vector<std::shared_ptr<counter_t>> get_bucket_counts();

// 序列化
void serialize(std::string& str);
```
## 例子
```cpp
  histogram_t h("test", "help", {5.0, 10.0, 20.0, 50.0, 100.0});
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
  std::cout << str;
  CHECK(str.find("test_count") != std::string::npos);
  CHECK(str.find("test_sum") != std::string::npos);
  CHECK(str.find("test_bucket{le=\"5") != std::string::npos);
  CHECK(str.find("test_bucket{le=\"+Inf\"}") != std::string::npos);
```

创建Histogram时需要指定桶(bucket)，采样点统计数据会落到不同的桶中，并且还需要统计采样点数据的累计总和(sum)以及次数的总和(count)。注意bucket 列表必须是有序的，否则构造时会抛异常。

Histogram统计的特点是：数据是累积的，比如由10， 100，两个桶，第一个桶的数据是所有值 <= 10的样本数据存在桶中，第二个桶是所有 <=100 的样本数据存在桶中，其它数据则存放在`+Inf`的桶中。

```cpp
  auto h = std::make_shared<histogram_t>(
      std::string("test"), std::string("help"), std::vector{10.0, 100.0});
  metric_t::regiter_metric(h);
  
  h->observe(5);
  h->observe(80);
  h->observe(120);
  
  std::string str;
  h.serialize(str);
  std::cout<<str;
```
第一个桶的数量为1，第二个桶的数量为2，因为小于等于100的样本有两个，observe(120)的时候，数据不会落到10或者100那两个桶里面，而是会落到最后一个桶`+Inf`中，所以`+Inf`桶的数量为3，因为小于等于+Inf的样本有3个。

序列化之后得到的指标结果为：
```
# HELP test help
# TYPE test histogram
test_bucket{le="10.000000"} 1.000000
test_bucket{le="100.000000"} 2.000000
test_bucket{le="+Inf"} 3.000000
test_sum 205.000000
test_count 3.000000
```

# summary
## api

```cpp
// Quantiles: 百分位和误差, 如：{{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}}
// 调用此函数则metric为静态metric 指标
summary_t(std::string name, std::string help, Quantiles quantiles);

// labels_name: 标签名，调用此函数则metric为动态metric 指标
summary_t(std::string name, std::string help, Quantiles quantiles, std::vector<std::string> labels_name);

// static_labels：静态标签，调用此函数则metric为静态metric 指标
summary_t(std::string name, std::string help, Quantiles quantiles, std::map<std::string, std::string> static_labels);

// 往summary_t插入数据，会自动计算百分位的数量
void observe(double value);

// 根据标签值(动态或静态的标签值，依据构造函数决定是动态还是静态metric)，往summary_t插入数据，会自动计算百分位的数量
void observe(std::vector<std::string> labels_value, double value);

// 获取分位数结果, sum 和count
async_simple::coro::Lazy<std::vector<double>> get_rates(double &sum,
                                                        uint64_t &count)
// 根据标签获取分位数结果, sum 和count
async_simple::coro::Lazy<std::vector<double>> get_rates(
    const std::vector<std::string> &labels_value, double &sum,
    uint64_t &count);

// 获取总和
async_simple::coro::Lazy<double> get_sum();

// 获取插入数据的个数
async_simple::coro::Lazy<uint64_t> get_count();

// 序列化
async_simple::coro::Lazy<void> serialize_async(std::string &str);
```

## 例子
```cpp
  summary_t summary{"test_summary",
                    "summary help",
                    {{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}}};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(1, 100);
  for (int i = 0; i < 50; i++) {
    summary.observe(distr(gen));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  std::string str;
  async_simple::coro::syncAwait(summary.serialize_async(str));
  std::cout << str;
  CHECK(async_simple::coro::syncAwait(summary.get_count()) == 50);
  CHECK(async_simple::coro::syncAwait(summary.get_sum()) > 0);
  CHECK(str.find("test_summary") != std::string::npos);
  CHECK(str.find("test_summary_count") != std::string::npos);
  CHECK(str.find("test_summary_sum") != std::string::npos);
  CHECK(str.find("test_summary{quantile=\"") != std::string::npos);
```
summary 百分位的计算相比其它指标是最耗时的，应该避免在关键路径上使用它以免对性能造成影响。

创建Summary时需要指定分位数和误差，分位数在0到1之间，左右都为闭区间，比如p50就是一个中位数，p99指中位数为0.99的分位数。
```cpp
  summary_t summary{"test_summary",
                    "summary help",
                    {{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}}};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(1, 100);
  for (int i = 0; i < 50; i++) {
    summary.observe(distr(gen));
  }

  std::string str;
  summary.serialize(str);
  std::cout << str;
```
输出:
```
# HELP test_summary summary help
# TYPE test_summary summary
test_summary{quantile="0.500000"} 45.000000
test_summary{quantile="0.900000"} 83.000000
test_summary{quantile="0.950000"} 88.000000
test_summary{quantile="0.990000"} 93.000000
test_summary_sum 2497.000000
test_summary_count 50
```

## 配置prometheus 前端
安装[prometheus](https://github.com/prometheus/prometheus)之后，打开其配置文件：prometheus.yml

修改要连接的服务端地址：
```
- targets: ["127.0.0.1:9001"]
```
然后启动prometheus，prometheus会定时访问`http://127.0.0.1:9001/metrics` 拉取所有指标数据。

在本地浏览器输入:127.0.0.1:9090, 打开prometheus前端，在前端页面的搜索框中输入指标的名称request_count之后就能看到table和graph 结果了。

# cinatra http server中启用内置的metric指标

http server 内置的指标：
```cpp
server_total_req: server总的请求数；
server_failed_req：server总的失败请求数；
server_total_fd：server使用的总的句柄数；
server_total_recv_bytes：server总共收到的字节数；
server_total_send_bytes：server总共发送的字节数；
server_req_latency：http 请求的延迟，从收到请求到发送响应的时间间隔
server_read_latency：http 读请求的延迟，读到完整的http数据的时间间隔
```

```cpp
coro_http_server server(1, 9001);
server.use_metrics("/metrics");//这个url默认就是/metrics，可以不填
```
在浏览器中输入`http://127.0.0.1:9001/metrics` 即可看到所有的指标。

查看当前server的client pool中有多少client，调用`pool.free_client_count()`

查看当前server内部线程池中有多少线程，调用`coro_io::get_total_thread_num()`