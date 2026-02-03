# coro_rpc 对barex的适配支持

coro_rpc内部适配了accl_barex库（调用其内部的send/on_recv方法）。该库可以在PPU，MLX，ERDMA等多种环境下通信。用户只需简单配置即可完成切换。

## barex相关配置

### barex socket

`barex_socket_t`是针对barex通信库的抽象。通过将其封装为socket（支持accept，connect，read,write）等操作，coro_rpc上层可以在保持相同代码逻辑的情况下，配置使用tcp/ibverbs/accl_barex等不同的协议栈。

对rpc用户，无需关系底层实现细节。只需调用client的init_barex方法，或者指定client的config中的socket_config字段，即可配置使用barex。

例如：

```cpp
  coro_rpc::coro_rpc_client client;
  [[maybe_unused]] bool is_ok = client.init_barex();
```

init_barex的参数是barex_socket_t::config_t。

```cpp
//coro_io::barex_socket_t::config_t
struct config_t {
  std::size_t buffer_size = 1024 * 1024; // 数据的分片发送大小，默认为1M。
  std::size_t max_buffer_cnt = 8;        // 数据的分片并发发送上限，默认为8。
  std::shared_ptr<barex_device_t> dev = nullptr; // 指定barex_socket使用的设备。
};
```

例如：

```cpp
  coro_rpc::coro_rpc_client client;
  coro_io::barex_socket_t::config_t barex_conf{
    .buffer_size = 1024 * 1024,
    .max_buffer_cnt = 8,
    .dev = nullptr
  };
  [[maybe_unused]] bool is_ok = client.init_barex(barex_conf);
```

对于rpc_server，我们需要通过在配置中使用barex_server_acceptor来指定启用barex。

```cpp
std::vector<std::unique_ptr<coro_io::server_acceptor_base>> acceptors;
  acceptors.emplace_back(std::make_unique<coro_io::barex_server_acceptor>(
      9001/*port*/, coro_io::barex_socket_t::config_t{
                .buffer_size = 256 * 1024,
                .dev = coro_io::get_global_barex_device()}));
  coro_rpc_server server(config_t{}, std::move(acceptors));
```

### barex device

`barex_device_t`是针对barex EIC网络设备的抽象。一台机器上可能有若干张barex网卡。我们提供了一系列接口，用于用户获取网络设备相关的信息。

```cpp
// 获取默认的barex设备（全局设备中index为0的barex device）
inline std::shared_ptr<barex_device_t> get_global_barex_device(
    const barex_device_t::config_t& config = {});
// 获取当前全部可用的barex设备列表。
inline std::vector<std::shared_ptr<barex_device_t>>& get_global_barex_devices(
    const barex_device_t::config_t& config = {}) {
  static auto manager = barex_device_t::get_manager();
  static auto barex_devices = barex_device_t::get_devices(manager, config);
  return barex_devices;
}
```

## 内存池与MR占用。

coro_rpc只会在发送数据时，临时性的从accl_barex的内存池中申请大小为`buffer_size`的MR，将数据拷贝到这块MR中。当数据足够大可被分为足够多片时，单连接可能的最大并发发送内存占用为`buffer_size`*`max_buffer_cnt`。当数据发送完毕后，MR会归还给内存池。

同一个`barex_device_t`下的所有`barex_socket_t`，共享同一个内存池。

接受数据也需要缓冲区，这部分内存占用由accl_barex内部申请。原则来说，每个内存池最多会申请64MB内存用于接收数据。

可以简单估算内存占用的上限：

最大rdma内存占用=rdma网卡数\*64MB+发送请求最大并发\*`buffer_size`*单连接最大并发（max_buffer_cnt）。

例如，假如一个server有1张网卡，最多接收1000个连接，buffer_size=128KB,max_buffer_cnt=4,则最大内存占用为1GB+64MB。

后续可以考虑参考barex内存池设计，限制同一个设备下所有连接的发送缓冲区最大不超过64MB（或放宽到256MB）。这样可以保证rdma的连接数不影响其最大内存占用，从而增强可用性。

