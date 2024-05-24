# Introduction to coro_rpc Client

## Base Usage

class `coro_rpc::coro_rpc_client` is the client side of coro_rpc, allowing users to send RPC requests to the server. 

Below, we will demonstrate the basic usage of rpc_client.

```cpp
using namespace async_simple;
using namespace coro_rpc;
int add(int a,int b);
Lazy<void> example() {
  coro_rpc_client client;
  coro_rpc::err_code ec = co_await client.connect("localhost:9001");
  if (ec) { /*check if connection error*/
    std::cout<<ec.message()<<std::endl;
    co_return;
  }
  rpc_result result = co_await client.call<add>(1,2);
  /*rpc_result is type of expected<T,rpc_error>, which T is rpc return type*/
  if (!result.has_value()) {
    /*call result.error() to get rpc error message*/
    std::cout<<"error code:"<<result.error().val()<< ", error message:"<<result.error().msg()<<std::endl;
    co_return;
  }
  assert(result.value()==3); /*call result.value() to  get rpc return type*/
}
```

You can use the `set_req_attachment` function to set the attachment for the current request. This is a piece of binary data that will be sent directly to the client without serialization. Similarly, you can also use `get_resp_attachment()` and `release_resp_attachment()` to retrieve the attachment returned by the RPC request.

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

By default, the RPC client will wait for 5 seconds after sending a request/establishing a connection. If no response is received after 5 seconds, it will return a timeout error. Users can also customize the wait duration by calling the `call_for` function.

```cpp
client.connect("127.0.0.1:9001", std::chrono::seconds{10});
auto result = co_await client.call_for<add>(std::chrono::seconds{10},1,2);
assert(result.value() == 3);
```

The duration can be any `std::chrono::duration` type, common examples include `std::chrono::seconds` and `std::chrono::milliseconds`. Notably, if the duration is set to zero, it indicates that the function call will never time out.

## SSL support

coro_rpc supports using OpenSSL to encrypt connections. After installing OpenSSL and importing yalantinglibs into your project with CMake's `find_package` or `fetch_content`, you can enable SSL support by setting the CMake option YLT_ENABLE_SSL=ON. Alternatively, you might manually add the YLT_ENABLE_SSL macro and manually link to OpenSSL.

Once SSL support has been enabled, users can invoke the `init_ssl` function before establishing a connection to the server. This will create an encrypted link between the client and the server. It’s important to note that the coro_rpc server must also be compiled with SSL support enabled, and the `init_ssl` method must be called to enable SSL support before starting the server.

```cpp
client.init_ssl("./","server.crt");
```


The first string represents the base path where the SSL certificate is located, the second string represents the relative path of the SSL certificate relative to the base path.

## Conversion and compile-time checking of RPC parameters

coro_rpc will perform compile-time checks on the validity of arguments during invocation. For example, for the following rpc function:

```cpp
inline std::string echo(std::string str) { return str; }
```

Next, when the current client invokes the rpc function:

```cpp
client.call<echo>(42); // Parameter does not match, compilation error
client.call<echo>(); // Missing parameter, compilation error
client.call<echo>("", 0); // Extra parameters, compilation error
client.call<echo>("hello, coro_rpc"); // The string literal can be converted to std::string, compilation succeeds
```

## Connect Option

The `coro_rpc_client` provides an `init_config` function for configuring connection options. The following code snippet lists the configurable options.

```cpp
using namespace coro_rpc;
using namespace std::chrono;
void set_config(coro_rpc_client& client) {
  client.init_config(config{
    .timeout_duration = 5s, // Timeout duration for requests and connections
    .host = "localhost", // Server hostname
    .port = "9001", // Server port
    .enable_tcp_no_delay = true, // Whether to disable socket-level delayed sending of requests
    /* The following options are available only when SSL support is activated */
    .ssl_cert_path = "./server.crt", // Path to the SSL certificate
    .ssl_domain = "localhost"
  });
}
```

## Calling Model

Each `coro_rpc_client` is bound to a specific IO thread. By default, it selects a connection via round-robin from the global IO thread pool. Users can also manually bind it to a specific IO thread.

```cpp
auto executor=coro_io::get_global_executor();
coro_rpc_client client(executor),client2(executor);
// Both clients are bound to the same IO thread.
```

Each time a coroutine-based IO task is initiated (such as `connect`, `call`, `send_request`), the client internally submits the IO event to the operating system. When the IO event is completed, the coroutine is then resumed on the bound IO thread to continue execution. For example, in the following code, the task switches to the IO thread for execution after calling connect.

```cpp
/*run in thread 1*/
coro_rpc_client cli;
co_await cli.connect("localhost:9001");
/*run in thread 2*/
do_something();
```

## Connection Pool and Load Balancing

`coro_io` offers a connection pool `client_pool` and a load balancer `channel`. Users can manage `coro_rpc`/`coro_http` connections through the `client_pool`, and can use `channel` to achieve load balancing among multiple hosts. For more details, please refer to the documentation of `coro_io`.

## Connection Reuse

The `coro_rpc_client` can achieve connection reuse through the `send_request` function. This function is thread-safe, allowing multiple threads to call the `send_request` method on the same client concurrently. The return value of the function is `Lazy<Lazy<async_rpc_result<T>>>`. The first `co_await` waits for the request to be sent, and the second `co_await` waits for the rpc result to return.


Connection reuse allows us to reduce the number of connections under high concurrency, eliminating the need to create new connections. It also improves the throughput of each connection.

Here's a simple example code snippet:

```cpp
using namespace coro_rpc;
using namespace async_simple::coro;
std::string_view echo(std::string_view);
Lazy<void> example(coro_rpc_client& client) {
  // Wait for the request to be fully sent
  Lazy<async_rpc_result> handler = co_await client.send_request<echo>("Hello");
  // Then wait for the server to return the RPC request result
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

We can call send_request multiple times to implement connection reuse:

```cpp
using namespace coro_rpc;
using namespace async_simple::coro;
std::string_view echo(std::string_view);
Lazy<void> example(coro_rpc_client& client) {
  std::vector<Lazy<async_rpc_result>> handlers;
  // First, send 10 requests consecutively
  for (int i=0;i<10;++i) {
    handlers.push_back(co_await client.send_request<echo>(std::to_string(i)));
  }
  // Next, wait for all the requests to return
  std::vector<async_rpc_result> results = co_await collectAll(std::move(handlers));
  for (int i=0;i<10;++i) {
    assert(results[i]->result() == std::to_string(i));
  }
  co_return;
}
```

### Attachment

When using the `send_request` method, since multiple requests might be sent simultaneously, we should not call the `set_req_attachment` method to send an attachment to the server, nor should we call the `get_resp_attachment` and `release_resp_attachment` methods to get the attachment returned by the server.

Instead, we can set the attachment when sending a request by calling the `send_request_with_attachment` function. Additionally, we can retrieve the attachment by calling the `->get_attachment()` and `->release_buffer()` methods of `async_rpc_result`.



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


### Execution order

When the called rpc function is a coroutine rpc function or a callback rpc function, the rpc requests may not necessarily be executed in order. The server might execute multiple rpc requests simultaneously.
For example, suppose there is the following code:

```cpp
using namespace async_simple::coro;
Lazy<void> sleep(int seconds) {
  co_await coro_io::sleep(1s * seconds);  // Yield the coroutine here
  co_return;
}
```

Server registration and startup:
```cpp
using namespace coro_rpc;
void start() {
  coro_rpc_server server(/* thread = */1,/* port = */ 8801);
  server.register_handler<sleep>();
  server.start();
}
```

The client consecutively calls the sleep function twice on the same connection, sleeping for 2 seconds the first time and 1 second the second time.
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
Under normal circumstances, handler3 will return first, followed by handler2, and finally handler1. Although the server only has one IO thread for executing rpc functions, the coroutine function will yield the coroutine when calling `coro_io::sleep`, thus ensuring that other connections will not be blocked.

### Socket Delayed Sending

When using connection reuse, you can try setting the option `enable_tcp_no_delay` to `false`. This allows the underlying implementation to batch multiple small requests together for sending, thereby increasing throughput, but it may lead to increased latency.

## Thread-safe

For multiple coro_rpc_client instances, they do not interfere with each other and can be safely called in different threads respectively.

When calling a single `coro_rpc_client` simultaneously in multiple threads, it is necessary to note that only some member functions are thread-safe, including `send_request()`, `close()`, `connect()`, `get_executor()`, `get_pipeline_size()`, `get_client_id()`, `get_config()`, etc. If the user has not called the `connect()` function again with an endpoint or hostname, then the `get_port()` and `get_host()` functions are also thread-safe.

It is important to note that the `call`, `get_resp_attachment`, `set_req_attachment`, `release_resp_attachment`, and `init_config` functions are not thread-safe and must not be called by multiple threads simultaneously. In this case, only `send_request` can be used for multiple threads to make concurrent requests over a single connection.
