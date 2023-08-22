coro_http_client 简介

# 基本用法

## 如何引入 coro_http_cient

coro_http_cient 是yalantinglibgs 的子库，yalantinglibs 是header only的，下载yalantinglibgs 库之后，在自己的工程中包含目录：

```c++
  include_directories(include)
  include_directories(include/ylt/thirdparty)
```

如果是gcc 编译器还需要设置以启用C++20 协程：
```c++
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
    #-ftree-slp-vectorize with coroutine cause link error. disable it util gcc fix.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize")
  endif()
```

最后在你的工程里引用coro_http_client 的头文件即可:

```c++
#include <iostream>
#include "ylt/coro_http/coro_http_client.hpp"

int main() {
    coro_http::coro_http_client client{};
    std::string uri = "http://cn.bing.com";
    auto result = client.get(uri);
    if (result.net_err) {
      std::cout << result.net_err.message() << "\n";
    }
    std::cout << result.status << "\n";

    result = client.post(uri, "hello", coro_http::req_content_type::json);
    std::cout << result.status << "\n";    
}
```

## http 同步请求

### http 同步请求接口
```c++
/// http 响应的结构体
/// \param net_err 网络错误，默认为空
/// \param status http 响应的状态码，正常一般为200
/// \param resp_body http 响应body，类型为std::string_view，如果希望保存到后面延迟处理则需要将resp_body 拷贝走
/// \param resp_headers http 响应头
/// \param eof http 响应是否结束，一般请求eof 为true，eof对于文件下载才有意义，
///            下载的中间过程中eof 为false，最后一个包时eof才为true)
struct resp_data {
  std::error_code net_err;
  int status;
  std::string_view resp_body;
  std::vector<std::pair<std::string, std::string>> resp_headers;
  bool eof;
};

/// \param uri http uri，如http://www.example.com
resp_data get(std::string uri);

enum class req_content_type {
  html,
  json,
  text,
  string,
  multipart,
  ranges,
  form_url_encode,
  octet_stream,
  xml,
  none
};

/// \param uri http uri，如http://www.example.com
/// \param content http 请求的body
/// \param content_type http 请求的content_type，如json、text等类型
resp_data post(std::string uri, std::string content,
                 req_content_type content_type);
```

### http 同步请求的用法
简单的请求一个网站一行代码即可:

```c++
coro_http_client client{};
auto result = client.get("http://www.example.com");
if(result.net_err) {
  std::cout << net_err.message() << "\n";
  return;
}

if(result.status == 200) {
  std::cout << result.resp_body << "\n";
}
```
请求返回之后需要检查是否有网络错误和状态码，如果都正常则可以处理获取的响应body和响应头了。

```c++
void test_sync_client() {
  {
    std::string uri = "http://www.baidu.com";
    coro_http_client client{};
    auto result = client.get(uri);
    assert(!result.net_err);
    print(result.resp_body);

    result = client.post(uri, "hello", req_content_type::json);
    print(result.resp_body);
  }

  {
    coro_http_client client{};
    std::string uri = "http://cn.bing.com";
    auto result = client.get(uri);
    assert(!result.net_err);
    print(result.resp_body);

    result = client.post(uri, "hello", req_content_type::json);
    print(result.resp_body);
  }
}
```

## http 异步请求接口

```c++
async_simple::coro::Lazy<resp_data> async_get(std::string uri);

async_simple::coro::Lazy<resp_data> async_post(
    std::string uri, std::string content, req_content_type content_type);
```
async_get和get 接口参数一样，async_post 和 post 接口参数一样，只是返回类型不同，同步接口返回的是一个普通的resp_data，而异步接口返回的是一个Lazy 协程对象。事实上，同步接口内部就是调用对应的协程接口，用法上接近，多了一个co_await 操作。

事实上你可以把任意异步协程接口通过syncAwait 方法同步阻塞调用的方式转换成同步接口，以同步接口get 为例：
```c++
resp_data get(std::string uri) {
  return async_simple::coro::syncAwait(async_get(std::move(uri)));
}
```

同步请求例子：
```c++
async_simple::coro::Lazy<void> test_async_client() {
  std::string uri = "http://www.baidu.com";

  {
    coro_http_client client{};
    auto data = co_await client.async_get(uri);
    print(data.status);

    data = co_await client.async_get(uri);
    print(data.status);

    data = co_await client.async_post(uri, "hello", req_content_type::string);
    print(data.status);
  }
}  
```

# https 请求
发起https 请求之前确保已经安装了openssl，并开启CINATRA_ENABLE_SSL 预编译宏：
```
option(CINATRA_ENABLE_SSL "Enable ssl support" OFF)
```
client 只需要调用init_ssl 方法即可，之后便可以和之前一样发起https 请求了。

```c++
const int verify_none = SSL_VERIFY_NONE;
const int verify_peer = SSL_VERIFY_PEER;
const int verify_fail_if_no_peer_cert = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
const int verify_client_once = SSL_VERIFY_CLIENT_ONCE;

  /// 
  /// \param base_path ssl 证书所在路径
  /// \param cert_file ssl 证书名称
  /// \param verify_mode 证书校验模式，默认不校验
  /// \param domain 校验的域名
  /// \return ssl 初始化是否成功
  bool init_ssl(const std::string &base_path = "", const std::string &cert_file = "",
                int verify_mode = asio::ssl::verify_none,
                const std::string &domain = "localhost");
```

```c++
#ifdef CINATRA_ENABLE_SSL
void test_coro_http_client() {
  coro_http_client client{};
  client.init_ssl("../../include/cinatra", "server.crt");
  auto data = client.get("https://www.bing.com");
  std::cout << data.resp_body << "\n";
  data = client.get("https://www.bing.com");
  std::cout << data.resp_body << "\n";

  std::string uri2 = "https://www.baidu.com";
  std::string uri3 = "https://cn.bing.com";
  coro_http_client client{};
  client.init_ssl("../../include/cinatra", "server.crt");
  data = co_await client.async_get(uri2);
  print(data.status);

  data = co_await client.async_get(uri3);
  print(data.status);  
}
#endif
```
如果没有ssl 证书，则init_ssl(); 参数不填。

# http 先连接再请求
前面介绍的get/post 接口传入uri，在函数内部会自动去连接服务器并发请求，一次性完成了连接和请求，如果希望将连接和请求分开程两个阶段，那么就可以先调用connect 接口再调用async_get 接口。

如果host 已经通过请求连接成功之后，后面发请求的时候只传入path 而不用传入完整的路径，这样可以获得更好的性能，coro_http_client 对于已经连接的host，当传入path 的时候不会再重复去解析已经解析过的uri。

```c++
async_simple::coro::Lazy<void> test_async_client() {
  std::string uri = "http://www.baidu.com";

  {
    coro_http_client client{};
    // 先连接
    auto data = co_await client.connect(uri);
    print(data.status);

    // 后面再发送具体的请求
    data = co_await client.async_get(uri);
    print(data.status);

    // 对于已经连接的host，这里可以只传入path，不需要传入完整的uri
    data = co_await client.async_post("/", "hello", req_content_type::string);
    print(data.status);
  }
}
```

# http 重连
当http 请求失败之后，这个http client是不允许复用的，因为内部的socket 都已经关闭了，除非你调用reconnect 去重连host，这样就可以服用http client 了。

```c++
  coro_http_client client1{};
  // 连接了一个非法的uri 会失败
  r = async_simple::coro::syncAwait(
      client1.async_http_connect("http//www.badurl.com"));
  CHECK(r.status != 200);

  // 通过重连复用client1
  r = async_simple::coro::syncAwait(client1.reconnect("http://cn.bing.com"));
  CHECK(client1.get_host() == "cn.bing.com");
  CHECK(client1.get_port() == "http");
  CHECK(r.status == 200);
```

# 其它http 接口
http_method
```c++
enum class http_method {
  UNKNOW,
  DEL,
  GET,
  HEAD,
  POST,
  PUT,
  PATCH,
  CONNECT,
  OPTIONS,
  TRACE
};
```

coro_http_client 提供了这些http_method 对应的请求接口:
```c++
async_simple::coro::Lazy<resp_data> async_delete(
    std::string uri, std::string content, req_content_type content_type);

async_simple::coro::Lazy<resp_data> async_get(std::string uri);

async_simple::coro::Lazy<resp_data> async_head(std::string uri);

async_simple::coro::Lazy<resp_data> async_post(
    std::string uri, std::string content, req_content_type content_type);

async_simple::coro::Lazy<resp_data> async_put(std::string uri,
                                              std::string content,
                                              req_content_type content_type);

async_simple::coro::Lazy<resp_data> async_patch(std::string uri);

async_simple::coro::Lazy<resp_data> async_http_connect(std::string uri);

async_simple::coro::Lazy<resp_data> async_options(std::string uri);

async_simple::coro::Lazy<resp_data> async_trace(std::string uri);
```
注意，async_http_connect 接口不是异步连接接口，它实际上是http_method::CONNECT 对应的接口，真正的异步连接接口connect 前面已经介绍过。

# 文件上传下载
除了http method 对应的接口之外，coro_http_client 还提供了常用文件上传和下载接口。

## chunked 格式上传
```c++
template <typename S, typename String>
async_simple::coro::Lazy<resp_data> async_upload_chunked(
    S uri, http_method method, String filename,
    std::unordered_map<std::string, std::string> headers = {});
```
method 一般是POST 或者PUT，filename 是带路径的文件名，headers 是请求头，这些参数填好之后，coro_http_client 会自动将文件分块上传到服务器，直到全部上传完成之后才co_return，中间上传出错也会返回。 

chunked 每块的大小默认为1MB，如果希望修改分块大小可以通过set_max_single_part_size 接口去设置大小，或者通过config 里面的max_single_part_size配置项去设置。

## multipart 格式上传
multipart 上传有两个接口，一个是一步实现上传，一个是分两步实现上传。

一步上传接口
```c++
async_simple::coro::Lazy<resp_data> async_upload_multipart(
    std::string uri, std::string name, std::string filename);
```
name 是multipart 里面的name 参数，filename 需要上传的带路径的文件名。client 会自动将文件分片上传，分片大小的设置和之前介绍的max_single_part_size 一样，默认分片大小是1MB。

一步上传接口适合纯粹上传文件用，如果要上传多个文件，或者既有字符串也有文件的场景，那就需要两步上传的接口。

两步上传接口
```c++
// 设置要上传的字符串key-value
bool add_str_part(std::string name, std::string content);
// 设置要上传的文件
bool add_file_part(std::string name, std::string filename);

// 上传
async_simple::coro::Lazy<resp_data> async_upload_multipart(std::string uri);
```
两步上传，第一步是准备要上传的字符串或者文件，第二步上传；

```c++
  std::string uri = "http://127.0.0.1:8090/multipart";

  coro_http_client client{};
  client.add_str_part("hello", "world");
  client.add_str_part("key", "value");
  auto result = async_simple::coro::syncAwait(client.async_upload_multipart(uri));
```

## multipart 格式下载
```c++
async_simple::coro::Lazy<resp_data> async_download(std::string uri,
                                                   std::string filename,
                                                   std::string range = "");
```
传入uri 和本地要保存的带路径的文件名即可，client 会自动下载并保存到文件中，直到全部下载完成。

## chunked 格式下载
chunked 下载接口和multipart 下载接口是相同的，client 内部会区分是chunked 格式还multipart 格式。

传入uri 和本地要保存的带路径的文件名即可，client 会自动下载并保存到文件中，直到全部下载完成。

## ranges 格式下载
ranges 下载接口和multipart、chunked 下载接口相同，需要填写ranges:
```c++
  coro_http_client client{};
  std::string uri = "http://uniquegoodshiningmelody.neverssl.com/favicon.ico";

  std::string filename = "test.txt";
  std::error_code ec{};
  std::filesystem::remove(filename, ec);
  resp_data result = async_simple::coro::syncAwait(
      client.async_download(uri, filename, "1-10,11-16"));

  std::string filename1 = "test1.txt";
  std::error_code ec{};
  std::filesystem::remove(filename1, ec);
  resp_data result = async_simple::coro::syncAwait(
      client.async_download(uri, filename1, "1-10"));
```
ranges 按照"m-n,x-y,..." 的格式填写，下载的内容将会保存到文件里。

## chunked\multipart\ranges 格式下载到内存
如果下载的数据量比较小，不希望放到文件里，希望放到内存里，那么直接使用async_get、async_post 等接口即可，chunked\multipart\ranges 等下载数据将会保存到resp_data.resp_body 中。

# http client 配置项
client 配置项：
```c++
  struct config {
    // 连接超时时间，默认8 秒
    std::optional<std::chrono::steady_clock::duration> conn_timeout_duration;
    // 请求超时时间，默认60 秒(包括连接时间和等待请求响应的时间)
    std::optional<std::chrono::steady_clock::duration> req_timeout_duration;
    // websocket 的安全key
    std::string sec_key;
    // chunked 下载/multipart 下载，chunked 上传/multipart上传时文件分片大小，默认1MB
    size_t max_single_part_size;
    // http 代理相关的设置
    std::string proxy_host;
    std::string proxy_port;
    std::string proxy_auth_username;
    std::string proxy_auth_passwd;
    std::string proxy_auth_token;
    // 是否启用tcp_no_delay
    bool enable_tcp_no_delay;
#ifdef CINATRA_ENABLE_SSL
    // 是否使用ssl
    bool use_ssl = false;
    // ssl 证书路径
    std::string base_path;
    // ssl 证书名称
    std::string cert_file;
    // ssl 校验模式
    int verify_mode;
    // ssl 校验域名
    std::string domain;
#endif
  };
```

把config项设置之后，调用init_config 设置http client 的参数。
```c++
coro_http_client client{};
cinatra::coro_http_client::config conf{.req_timeout_duration = 60s};
client.init_config(conf);
auto r = async_simple::coro::syncAwait(
    client.async_http_connect("http://www.baidu.com"));
```
# websocket
websocket 的支持需要3步：
- 设置读websocket 数据的回调函数；
- 连接服务器；
- 发送websocket 数据；

设置websocket 读数据接口:
```c++
void on_ws_msg(std::function<void(resp_data)> on_ws_msg);
```
websocket 连接服务器接口:
```c++
async_simple::coro::Lazy<bool> async_ws_connect(std::string uri);
```
websocket 发送数据接口：
```c++
enum opcode : std::uint8_t {
  cont = 0,
  text = 1,
  binary = 2,
  rsv3 = 3,
  rsv4 = 4,
  rsv5 = 5,
  rsv6 = 6,
  rsv7 = 7,
  close = 8,
  ping = 9,
  pong = 10,
  crsvb = 11,
  crsvc = 12,
  crsvd = 13,
  crsve = 14,
  crsvf = 15
};

/// 发送websocket 数据
/// \param msg 要发送的websocket 数据
/// \param need_mask 是否需要对数据进行mask，默认会mask
/// \param op opcode 一般为text、binary或 close 等类型
async_simple::coro::Lazy<resp_data> async_send_ws(std::string msg,
                                                  bool need_mask = true,
                                                  opcode op = opcode::text);
```

websocket 例子:

```c++
  coro_http_client client;
  // 连接websocket 服务器
  async_simple::coro::syncAwait(
      client.async_ws_connect("ws://localhost:8090"));

  std::string send_str(len, 'a');
  // 设置读数据回调
  client.on_ws_msg([&, send_str](resp_data data) {
    if (data.net_err) {
      std::cout << "ws_msg net error " << data.net_err.message() << "\n";
      return;
    }

    std::cout << "ws msg len: " << data.resp_body.size() << std::endl;
    REQUIRE(data.resp_body.size() == send_str.size());
    CHECK(data.resp_body == send_str);
  });

  // 发送websocket 数据
  async_simple::coro::syncAwait(client.async_send_ws(send_str));
```
