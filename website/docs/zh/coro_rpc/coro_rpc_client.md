# coro_rpc客户端介绍

## 基本使用

类`coro_rpc::coro_rpc_client`是coro_rpc的客户端，用户可以通过它向服务器发送rpc请求。

下面我们展示rpc_client的基本使用方法。

```cpp
using namespace async_simple;
using namespace coro_rpc;
int add(int a,int b);
Lazy<void> example() {
  coro_rpc_client client;
  coro_rpc::err_code ec = co_await client.connect("localhost:9001");
  if (ec) { /*判断连接是否出错*/
    std::cout<<ec.message()<<std::endl;
    co_return;
  }
  rpc_result result = co_await client.call<add>(1,2);
  /*rpc_result是一个expected<T,rpc_error>类型，其中T为rpc返回值*/
  if (!result.has_value()) {
    /*调用result.error()获取rpc错误信息*/
    std::cout<<"error code:"<<result.error().val()<< ", error message:"<<result.error().msg()<<std::endl;
    co_return;
  }
  assert(result.value()==3); /*调用result.value()获取rpc返回值*/
}
```

我们可以通过`set_req_attachment`函数设置本次请求的attachment，这是一段不会经过序列化直接发送给客户端的二进制数据。同样的，我们也可以使用`get_resp_attachment()`和`release_resp_attachment()`来获取rpc请求返回的attachment。

```cpp
using namespace async_simple;
using namespace coro_rpc;
void attachment_echo() {
  auto ctx=coro_rpc::get_context();
  ctx->set_resp_attachment(ctx->get_req_attachment());
}
Lazy<std::string> example(coro_rpc_client& client, std::string_view attachment) {
  client.set_req_attachment(attachment);
  rpc_result result = co_await client.call<attachment_echo>();
  if (result.has_value()) {
    assert(result.get_resp_attachment()==attachment);
    co_return std::move(result.release_resp_attachment());
  }
  co_return "";
}
```

默认情况下，rpc客户端发送请求/建立连接后会等待5秒，如果5秒后仍未收到响应，则会返回超时错误。
用户也可以通过调用`call_for`函数自定义等待的时长。

```cpp
client.connect("127.0.0.1:9001", std::chrono::seconds{10});
auto result = co_await client.call_for<add>(std::chrono::seconds{10},1,2);
assert(result.value() == 3);
```

时长可以是任一的`std::chrono::duration`类型，常见的如`std::chrono::seconds`,`std::chrono::millseconds`。
特别的，如果时长为0，代表该函数调用永远也不会超时。

## SSL支持

coro_rpc支持使用openssl对连接进行加密。在安装openssl并使用cmake find_package/fetch_content 将yalantinglibs导入到你的工程后，可以打开cmake选项`YLT_ENABLE_SSL=ON`启用ssl支持。或者，你也可以手动添加宏`YLT_ENABLE_SSL`并手动链接openssl。

当启用ssl支持后，用户可以调用`init_ssl`函数，然后再连接到服务器。这会使得客户端与服务器之间建立加密的链接。需要注意的是，coro_rpc服务端在编译时也必须启用ssl支持，并且在启动服务器之前也需要调用`init_ssl`方法来启用SSL支持。

```cpp
client.init_ssl("./","server.crt");
```

第一个字符串代表SSL证书所在的基本路径，第二个字符串代表SSL证书相对于基本路径的相对路径。

当建立连接时，客户端会使用该证书校验服务端发来的证书，以避免中间人攻击。因此，客户端必须持有服务端使用的证书或其根证书。

## RPC参数的转换与编译期检查

coro_rpc会在调用的时候对参数的合法性做编译期检查，比如，对于如下rpc函数：

```cpp
inline std::string echo(std::string str) { return str; }
```

接下来，当前client调用rpc函数时：

```cpp
client.call<echo>(42);// The argument does not match, a compilation error occurs.
client.call<echo>();// Missing argument, compilation error occurs.
client.call<echo>("", 0);// There are too many arguments, a compilation error occurs.
client.call<echo>("hello, coro_rpc");// The string literal can be converted to std::string, compilation succeeds.
```

## 连接选项

coro_rpc_client提供了`init_config`函数，用于配置连接选项。下面这份代码会列出可配置的选项。

```cpp
using namespace coro_rpc;
using namespace std::chrono;
void set_config(coro_rpc_client& client) {
  client.init_config(config{
    .timeout_duration = 5s //请求和连接的超时时间
    .host = "localhost" // 服务器域名
    .port = "9001" // 服务器端口
    .enable_tcp_no_delay  = true //是否禁止socket底层延迟发送请求
    /*以下选项只在激活ssl支持后可用*/
    .ssl_cert_path = "./server.crt" //ssl证书路径
    .ssl_domain = "localhost" 
  });
}
```

## 调用模型

每一个`coro_rpc_client`都会绑定到某个IO线程上，默认通过轮转法从全局IO线程池中选择一个连接，用户也可以手动绑定到特定的IO线程上。

```cpp
auto executor=coro_io::get_global_executor();
coro_rpc_client client(executor),client2(executor);
// 两个客户端都被绑定到同一个io线程上
```

每次发起一个基于协程的IO任务（如`connect`,`call`,`send_request`），客户端内部会将IO事件提交给操作系统，当IO事件完成后，再将协程恢复到绑定的IO线程上继续执行。

例如以下代码，调用connect之后任务将切换到IO线程执行。

```cpp
/*run in thread 1*/
coro_rpc_client cli;
co_await cli.connect("localhost:9001");
/*run in thread 2*/
do_something();
```

## 连接池与负载均衡

`coro_io`提供了连接池`client_pool`与负载均衡器`channel`。用户可以通过连接池`client_pool`来管理`coro_rpc`/`coro_http`连接，可以使用`channel`实现多个host之间的负载均衡。具体请见`coro_io`的文档。

## 连接复用

`coro_rpc_client` 可以通过 `send_request`函数实现连接复用。该函数是线程安全的，允许多个线程同时调用同一个client的 `send_request`方法。该函数返回值为`Lazy<Lazy<async_rpc_result<T>`。第一次`co_await`可以等待请求发送，再次`co_await`则等待rpc返回结果。


连接复用允许我们在高并发下减少连接的个数，无需创建新的连接。同时也能提高每个连接的吞吐量。

下面是一段简单的示例代码：

```cpp
using namespace coro_rpc;
using namespace async_simple::coro;
std::string_view echo(std::string_view);
Lazy<void> example(coro_rpc_client& client) {
  //先等待请求发送完毕
  Lazy<async_rpc_result> handler = co_await client.send_request<echo>("Hello");
  //然后等待服务器返回rpc请求结果
  async_rpc_result result = co_await handler;
  if (result) {
    assert(result->result() == "Hello");
  }
  else {
    // error handle
    std::cout<<result.error().msg()<<std::endl;
  }
}
```

我们可以多次调用send_request实现连接复用：

```cpp
using namespace coro_rpc;
using namespace async_simple::coro;
std::string_view echo(std::string_view);
Lazy<void> example(coro_rpc_client& client) {
  std::vector<Lazy<async_rpc_result>> handlers;
  //首先连续发送10个请求
  for (int i=0;i<10;++i) {
    handlers.push_back(co_await client.send_request<echo>(std::to_string(i)));
  }
  //接下来等待所有的请求返回
  std::vector<async_rpc_result> results = co_await collectAll(std::move(handlers));
  for (int i=0;i<10;++i) {
    assert(results[i]->result() == std::to_string(i));
  }
  co_return;
}
```

### Attachment

使用`send_request`方法时，由于可能同时发送多个请求，因此我们不能调用`set_req_attachment`方法向服务器发送attachment，同样也不能调用`get_resp_attachment`和`release_resp_attachment`方法来获取服务器返回的attachment。

我们可以通过调用`send_request_with_attachment`函数，在发送请求时设置attachment。我们也可以通过调用async_rpc_result的`->get_attachment()`方法和`->release_buffer()`方法来获取attachment。

```cpp
using namespace coro_rpc;
using namespace async_simple::coro;
int add(int a, int b);
Lazy<std::string> example(coro_rpc_client& client) {
  async_rpc_result result = co_await co_await client.send_request_with_attachment<echo>("Hello", 1, 2);
  assert(result->result() == 3);
  assert(result->get_attachment() == "Hello");
  co_return std::move(result->release_buffer().resp_attachment_buf_);
}
```


### 执行顺序

当调用的rpc函数是协程rpc函数或回调rpc函数时，rpc请求不一定会按顺序执行，服务端可能会同时执行多个rpc请求。

例如，假如有以下代码：

```cpp
using namespace async_simple::coro;
Lazy<void> sleep(int seconds) {
  co_await coro_io::sleep(1s * seconds);  // 在此处让出协程
  co_return;
}
```

服务器注册并启动：
```cpp
using namespace coro_rpc;
void start() {
  coro_rpc_server server(/* thread = */1,/* port = */ 8801);
  server.register_handler<sleep>();
  server.start();
}
```

客户端连续在同一个连接上调用两次sleep函数，第一次sleep2秒，第二次sleep1秒。
```cpp
using namespace async_simple::coro;
using namespace coro_rpc;
Lazy<void> call() {
  coro_rpc_client cli,cli2;
  co_await cli.connect("localhost:8801");
  co_await cli2.connect("localhost:8801");
  auto handler1 = co_await cli.send_request<sleep>(2);
  auto handler2 = co_await cli.send_request<sleep>(1);
  auto handler3 = co_await cli2.send_request<sleep>(0);
  handler2.start([](auto&&){
    std::cout<<"handler2 return"<<std::endl;
  });
  handler3.start([](auto&&){
   d::cout<<"handler3 return"<<std::endl;
  });
  co_await handler1;
  std::cout<<"handler1 return"<<std::endl;
}
```
正常情况下handler3会先返回，随后是handler2，最后是handler1。尽管服务器只有一个IO线程用于执行rpc函数，但协程函数会在调用`coro_io::sleep`时让出该协程，从而保证其他连接不会被阻塞。

### socket延迟发送

当使用连接复用时，可以尝试将选项中的`enable_tcp_no_delay`设为`false`,这允许底层实现将多个小请求打包后一起发送，从而提高吞吐量，但是可能会导致延迟上升。

## 线程安全

对于多个coro_rpc_client实例，它们之间互不干扰，可以分别在不同的线程中安全的调用。

单个coro_rpc_client在多个线程同时调用时需要注意，只有部分成员函数是线程安全的，包括`send_request()`,`close()`,`connect()`,`get_executor()`,`get_pipeline_size()`,`get_client_id()`,`get_config()`等。如果用户没有重新调用connect()函数并传入endpoint或hostname，那么`get_port()`,`get_host()`函数也是线程安全的。

需要注意，`call`,`get_resp_attachment`,`set_req_attachment`,`release_resp_attachment`和`init_config`函数均不是线程安全的，禁止多个线程同时调用。此时只能使用`send_request`实现多个线程并发请求同一个连接。
