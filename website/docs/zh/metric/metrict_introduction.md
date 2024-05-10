# metric 介绍
metric 用于统计应用程序的各种指标，这些指标被用于系统见识和警报，常见的指标类型有四种：Counter、Guage、Histogram和Summary，这些指标遵循[Prometheus](https://hulining.gitbook.io/prometheus/introduction)的数据格式。

## Counter 计数器类型
Counter是一个累计类型的数据指标，它代表单调递增的计数器，其值只能在重新启动时增加或重置为 0。例如，您可以使用计数器来表示已响应的请求数，已完成或出错的任务数。

不要使用计数器来显示可以减小的值。例如，请不要使用计数器表示当前正在运行的进程数；使用 gauge 代替。

## Gauge 数据轨迹类型
Gauge 是可以任意上下波动数值的指标类型。

Gauge 通常用于测量值，例如温度或当前的内存使用量，还可用于可能上下波动的"计数"，例如请求并发数。

如：
```
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
```
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
```
# HELP prometheus_tsdb_wal_fsync_duration_seconds Duration of WAL fsync.
# TYPE prometheus_tsdb_wal_fsync_duration_seconds summary
prometheus_tsdb_wal_fsync_duration_seconds{quantile="0.5"} 0.012352463
prometheus_tsdb_wal_fsync_duration_seconds{quantile="0.9"} 0.014458005
prometheus_tsdb_wal_fsync_duration_seconds{quantile="0.99"} 0.017316173
prometheus_tsdb_wal_fsync_duration_seconds_sum 2.888716127000002
prometheus_tsdb_wal_fsync_duration_seconds_count 216
```

# 如何使用metric功能

## 使用counter指标统计http 请求总数
http 请求数量随着时间推移是不断增加的，不可能会减少，因此使用counter类型的指标是合适的，如果数量可能会减少则应该使用guage类型的指标。

### 创建counter 对象

counter 的构造函数
```cpp
counter_t(std::string name, std::string help,
            std::vector<std::string> labels_name = {});
```
name: counter 的名称；
help: counter 的帮助信息；
labels_name: 标签的名称列表，默认为空。标签是一个键值对，由标签名称和标签值组成，稍后会在例子中介绍。

如果希望创建一个统计http 请求数量的counter，可以这样创建：
```cpp
auto c = std::make_shared<counter_t>("request_count", "request count", std::vector{"method", "url"});
```
counter 的名称为request_count，帮助信息为request count，标签名为method 和url，标签的值是动态增加的，比如
```
{method = "GET", url = "/"} 10
{method = "POST", url = "/test"} 20
```
method的和url的值就是标签的值，这是动态的，标签之后跟着count数量，第一行表示`GET /`请求的数量为10，`GET /test`请求的数量为20。

如果创建counter的时候不设置标签名称，则counter使用空的标签列表为默认标签。

### 增加counter
创建counter之后需要增加它的值，调用其inc成员函数即可。

```cpp
void inc(); //#1 如果发生错误会抛异常
void inc(const std::vector<std::string> &labels_value, double value = 1); //#2 如果发生错误会抛异常
```
#1 重载函数给默认标签的counter增加数量；

#2 重载函数给指定标签值的counter增加数量，注意：如果标签值列表和创建时标签名称列表的数量没有匹配上则会抛异常。

统计http server 请求数量：
```cpp
  coro_http_server server(1, 9001);
  server.set_http_handler<GET>(
      "/get", [&](coro_http_request &req, coro_http_response &resp) {
        resp.set_status_and_content(status_type::ok, "ok");
        c->inc({std::string(req.get_method()), std::string(req.get_url())}); //#1
      });
  server.sync_start();
```
当收到`/get`请求时，代码#1 调用counter的inc来增加请求数量，标签值是请求的method 名和url。

## 注册counter
一个应用可能需要统计多个指标，这些指标需要放到一个map中便于管理，比如前端需要拉取所有指标的数据则需要遍历map获取每个指标的详细数据。

注册指标调用：
```cpp
metric_t::regiter_metric(c);
```

## 返回统计结果给前端
前端一般是prometheus 前端，配置它需要访问http server地址，默认会通过`/metrics` url来访问所有的指标数据。所以需要给http server 提供`/metrics`的http handler用于响应prometheus 前端的请求。
```cpp
  coro_http_server server(1, 9001);
  server.set_http_handler<GET>(
      "/get", [&](coro_http_request &req, coro_http_response &resp) {
        resp.set_status_and_content(status_type::ok, "ok");
        c->inc({std::string(req.get_method()), std::string(req.get_url())}); //#1
      });

  server.set_http_handler<GET, POST>(
       "/metrics", [](coro_http_request &req, coro_http_response &resp) {
        std::string str;
        auto metrics = metric_t::collect(); //#1 获取所有的指标对象
        for (auto &m : metrics) {
            m->serialize(str); // #2 序列化指标
        }

        resp.set_status_and_content(status_type::ok, std::move(str));
    });
  server.sync_start();  
```
当前端访问`/metrics` 接口时，通过代码#1 `metric_t::collect()`来获取所有的指标对象，代码#2 `serialize(str)` 将指标详情序列化到str，然后返回给前端。

完整的代码：
```cpp
void use_counter() {
  auto c = std::make_shared<counter_t>("request_count", "request count", std::vector{"method", "url"});
  metric_t::regiter_metric(c);
  coro_http_server server(1, 9001);
  server.set_http_handler<GET>(
      "/get", [&](coro_http_request &req, coro_http_response &resp) {
        resp.set_status_and_content(status_type::ok, "ok");
        c->inc({std::string(req.get_method()), std::string(req.get_url())}); //#1
   });

  server.set_http_handler<GET, POST>(
       "/metrics", [](coro_http_request &req, coro_http_response &resp) {
        std::string str;
        auto metrics = metric_t::collect();
        for (auto &m : metrics) {
            m->serialize(str);
        }

        resp.set_status_and_content(status_type::ok, std::move(str));
    });
  server.sync_start(); 
}
```

## 配置prometheus 前端
安装[prometheus](https://github.com/prometheus/prometheus)之后，打开其配置文件：prometheus.yml

修改要连接的服务端地址：
```
- targets: ["127.0.0.1:9001"]
```
然后启动prometheus，prometheus会定时访问`http://127.0.0.1:9001/metrics` 拉取所有指标数据。

在本地浏览器输入:127.0.0.1:9090, 打开prometheus前端，在前端页面的搜索框中输入指标的名称request_count之后就能看到table和graph 结果了。

## 使用guage
guage和counter的用法几乎一样，guage比counter多了一个dec方法用来减少数量。

创建一个guage:
```cpp
auto g = std::make_shared<gauge_t>("not_found_request_count",
                                         "not found request count",
                                         std::vector{"method", "code", "url"});
metric_t::regiter_metric(g);
```
后面根据自己的需要在业务函数中inc或者dec即可。

## 使用Histogram
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

## 使用Summary
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