
## v0.6.1-internal Release Notes

### 🚀 New Features

- **[coro_io] CUDA Device PCI Bus ID**：`cuda_device` 新增 `pci_bus_id()` 接口，初始化时缓存结果，方便设备识别与管理。
- **[coro_rpc] GDR Example**：新增 GPU Direct RDMA (GDR) 示例，展示 GPU 内存直接 RDMA 传输的完整用法。
- **[coro_io/coro_rpc] Client Pool & Load Balancer `clear()` 接口**：`client_pool` 和 `load_balancer` 新增 `clear()` 方法，支持主动清理空闲连接并返回清理数量。
- **[coro_rpc] Server `connection_count()` 接口**：`coro_rpc_server` 新增 `connection_count()` 方法，可查询当前活跃连接数。
- **[ibverbs] GID 自动选择重构**：基于优先级的 GID 自动选择逻辑，提升 RDMA 设备初始化的智能性和可靠性。

### 🐛 Bug Fixes

- **[coro_io][cuda] 修复 `cuda_stream` 初始化错误**：修正 CUDA stream 未正确初始化的问题。
- **[coro_rpc][coro_io] 修复 GDR 崩溃**：修复 GPU Direct RDMA 场景下的 crash 问题。
- **[coro_io][coro_rpc] 修复 CUDA 事件内存泄漏**：改进 `cuda_event_t` 资源管理，优化 stream event deleter 实现。
- **[coro_rpc][coro_http] 修复 OpenSSL < 1.1 编译失败**：兼容低版本 OpenSSL 的编译问题。

### 🔧 Improvements

- **[barex] 编译兼容性**：新增宏保护，在不支持 barex 的环境中避免编译失败。
- **日志级别调整**：CacheFS 相关日志从 info 调整为 warning，减少日志噪音。