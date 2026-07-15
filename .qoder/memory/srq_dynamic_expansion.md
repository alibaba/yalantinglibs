# SRQ (Shared Receive Queue) 完整实现总结

## 概述

SRQ 允许同一 `ib_device` 上的所有 QP 共享一组接收缓冲区，避免每个 QP 独立维护接收队列。缓冲区由设备级管理器统一管理，根据流量动态扩缩容。

涉及 3 个提交：
1. `d67835ab` — SRQ 基础支持（QP 挂载 SRQ、接收路径改造）
2. `30986eab` — 延迟回收机制（空闲超时销毁 SRQ + signal 取消）
3. `d7fb542b` — 动态扩缩容 + 自动探测 + 配置优化

## 涉及文件

| 文件 | 职责 |
|------|------|
| `include/ylt/coro_io/ibverbs/ib_buffer.hpp` | SRQ 数据结构 + buffer 管理器声明 |
| `include/ylt/coro_io/ibverbs/ib_device.hpp` | SRQ 生命周期管理 + buffer 管理器实现 |
| `include/ylt/coro_io/ibverbs/ib_socket.hpp` | SRQ 在 socket 层的集成（QP 创建、收发路径） |
| `src/coro_io/tests/ibverbs/ib_socket_pressure_test.cpp` | 压测配置 |
| `website/docs/{zh,en}/coro_rpc/coro_rpc_rdma.md` | 文档 |

---

## 1. 数据结构 (`ib_buffer.hpp`)

### `ib_srq_recv_entry_t` — 每个 SRQ buffer 的 entry

```cpp
struct ib_srq_recv_entry_t {
  ib_buffer_t buffer;                    // RDMA 注册的内存
  std::atomic<bool> active{false};       // CAS 锁：false=空闲，true=占用
  ib_srq_recv_entry_t() = default;
  explicit ib_srq_recv_entry_t(ib_buffer_t b) : buffer(std::move(b)) {}
};
```

`active` 标志用于 `find_free_slot()` 的 CAS 无锁分配。

### `ib_srq_buffer_manager_t` — SRQ buffer 管理器

#### config_t

```cpp
struct config_t {
  uint64_t max_buffer_memory;   // 总内存上限
  uint32_t srq_max_wr;          // SRQ 队列深度
  uint32_t srq_max_sge;         // 固定 1
  size_t buffer_size;           // 每块 buffer 大小
  uint32_t buffer_block = 0;    // 初始/最小 buffer 数
};
```

#### 成员变量

```cpp
ib_device_t& device_;
std::vector<std::unique_ptr<ib_srq_recv_entry_t>> slots_;  // 环形槽位 (watermark*2)
std::unique_ptr<ibv_srq, ib_deleter> srq_;                 // 硬件 SRQ
config_t config_;
uint32_t watermark_;          // max(total_count) = min(max_mem/buf_size, srq_max_wr)
uint32_t buffer_block_ = 0;   // 初始/最小 buffer 数
alignas(64) std::atomic<uint32_t> posted_count_{0};  // SRQ 中空闲 buffer 数
alignas(64) std::atomic<uint32_t> total_count_{0};   // 活跃 buffer 总数 (posted+busy)
std::atomic<uint32_t> scan_pos_{0};                   // find_free_slot 扫描起点
std::atomic<bool> initialized_{false};                // init() 幂等保护
```

**关键设计**：`posted_count_` 和 `total_count_` 用 `alignas(64)` 分离到不同 cache line，避免 false sharing。

#### 三级水位系统

| 变量 | 含义 | 默认值 |
|------|------|--------|
| `buffer_block_` | 初始/最小（缩容下限） | 32 |
| `total_count_` | 当前活跃数（动态变化） | 初始 32，范围 [32, 1024] |
| `watermark_` | 绝对上限 | `min(256MB/256KB, 4096)` = 1024 |

---

## 2. Buffer 管理器实现 (`ib_device.hpp`)

### 构造函数

```cpp
ib_srq_buffer_manager_t(ib_device_t& device, const config_t& conf)
```

1. 计算 `watermark_ = min(max_buffer_memory / buffer_size, srq_max_wr)`，floor 1
2. 计算 `buffer_block_`：若 config 为 0 则用 `watermark_`，上限 `watermark_`
3. 调用 `ibv_create_srq(device.pd(), &srq_init_attr)` 创建硬件 SRQ
4. 失败则抛 `std::system_error`（被 device 构造函数 catch）

**构造函数不分配 buffer**——只创建硬件队列。

### `init()` — 幂等初始化

```cpp
void init() {
  bool expected = false;
  if (!initialized_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
    return;  // 已初始化
  // 分配 watermark*2 个 slot 壳（不分配 buffer）
  // 投递 buffer_block_ 个 buffer 到 SRQ
}
```

CAS 保证多线程并发调用安全。slot 壳是预分配的空 `ib_srq_recv_entry_t`（无 buffer），约 64 字节/个。

### `find_free_slot()` — 无锁槽位分配

```cpp
ib_srq_recv_entry_t* find_free_slot() {
  uint32_t pos = scan_pos_.load(std::memory_order_acquire);
  uint32_t start = pos;
  do {
    auto& slot = slots_[pos];
    bool expected = false;
    if (slot->active.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
      scan_pos_.store((pos + 1) % slots_.size(), std::memory_order_release);
      return slot.get();
    }
    pos = (pos + 1) % slots_.size();
  } while (pos != start);
  return nullptr;  // 环满
}
```

### `expand(count)` — 扩容

```cpp
void expand(uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    // 1. CAS 递增 total_count_（上限 watermark_）
    // 2. find_free_slot() 获取空闲槽位
    // 3. 从内存池获取 buffer
    // 4. post_entry() 投递到 SRQ
    // 5. posted_count_++
    // 失败时回滚 total_count_ 和 active 标志
  }
}
```

### `dec_posted_count()` — 接收完成时触发扩容

```cpp
void dec_posted_count() noexcept {
  uint32_t posted = posted_count_.fetch_sub(1, std::memory_order_acq_rel) - 1;
  uint32_t total = total_count_.load(std::memory_order_acquire);
  if (posted < total / 3 && total < watermark_) {
    uint32_t expand_count = std::min(total / 3, watermark_ - total);
    if (expand_count > 0) expand(expand_count);
  }
}
```

**扩容条件**：`posted < total / 3`（空闲 buffer 太少）

### `replenish(entry, consumed_buffer)` — 消费后归还 + 缩容

```cpp
void replenish(ib_srq_recv_entry_t* entry, ib_buffer_t consumed_buffer) {
  uint32_t posted = posted_count_.load(std::memory_order_acquire);
  uint32_t total = total_count_.load(std::memory_order_acquire);

  // 缩容：posted > total*2/3 且 total > buffer_block_
  if (posted > total * 2 / 3) {
    // CAS 递减 total_count_（下限 buffer_block_）
    // 标记 entry inactive，归还 buffer，直接返回
  }

  // 正常归还：重新投递 consumed_buffer
  entry->buffer = std::move(consumed_buffer);
  // CAS 递增 posted_count_，post_entry()

  // 再次检查扩容
  if (posted < total / 3 && total < watermark_) expand(...);
}
```

**缩容条件**：`posted > total * 2/3` 且 `total > buffer_block_`（空闲 buffer 太多）

### `repost(entry)` — 错误路径重投

```cpp
void repost(ib_srq_recv_entry_t* entry) {
  if (post_entry(entry))
    posted_count_.fetch_add(1, std::memory_order_release);
}
```

---

## 3. Device 层 SRQ 生命周期 (`ib_device.hpp`)

### config_t 默认值

```cpp
bool use_srq = true;                              // 默认开启，自动探测
uint64_t max_srq_buffer_memory = 256 * 1024 * 1024; // 256MB
uint32_t srq_max_wr = 4096;
uint32_t srq_idle_timeout_ms = 3000;               // 空闲销毁延迟
uint32_t srq_buffer_block = 32;                    // 初始/最小 buffer 数
```

### 自动探测（构造函数）

```cpp
if (conf.use_srq) {
  srq_enabled_ = true;
  // 计算 srq_conf_, srq_watermark_, srq_idle_timeout_ms_
  try {
    srq_manager_ = std::make_shared<ib_srq_buffer_manager_t>(*this, srq_conf_);
    // 只创建硬件 SRQ，不分配 buffer（init() 延迟到 ensure_srq()）
  } catch (const std::system_error& e) {
    ELOG_WARN << "SRQ creation failed, falling back to per-QP mode: " << e.what();
    srq_enabled_ = false;
    srq_manager_.reset();
  }
}
```

`srq_enabled_` 是 `bool`（构造后不可变，无需 atomic）。

### `ensure_srq()` — 确保 SRQ 就绪

```cpp
void ensure_srq() {
  if (!srq_enabled_) return;
  if (srq_manager_) {
    srq_manager_->init();  // 幂等，分配 buffer
    return;
  }
  // 空闲超时销毁后重建
  srq_manager_ = std::make_shared<ib_srq_buffer_manager_t>(*this, srq_conf_);
  srq_manager_->init();
}
```

### 空闲延迟回收

```cpp
void on_qp_created() {
  if (!srq_enabled_) return;
  active_qp_count_.fetch_add(1, std::memory_order_acq_rel);
}

void on_qp_destroyed(executor) {
  if (!srq_enabled_) return;
  uint32_t prev = active_qp_count_.fetch_sub(1, std::memory_order_acq_rel);
  if (prev == 1) {  // 最后一个 QP 销毁
    if (!destruction_pending_.exchange(true)) {
      // 启动 srq_destruction_lazy() 协程
    }
  }
}

Lazy<void> srq_destruction_lazy() {
  co_await sleep_for(srq_idle_timeout_ms_);
  destruction_pending_.store(false);
  if (active_qp_count_ == 0) {
    srq_manager_.reset();  // 销毁 SRQ
  }
}
```

如果延迟期间有新 QP 创建，`ensure_srq()` 发现 `srq_manager_` 存在则复用，`srq_destruction_lazy()` 超时后检查 `active_qp_count_ > 0` 则不销毁。

---

## 4. Socket 层 SRQ 集成 (`ib_socket.hpp`)

### shared state 快照

```cpp
struct ib_socket_shared_state_t {
  ib_srq_recv_entry_t* srq_entry_ = nullptr;  // 当前 SRQ entry
  bool use_srq_ = false;                       // 从 device 快照
  // 构造时：use_srq_(device->use_srq())
};
```

### QP 能力配置 (`init()`)

```cpp
if (conf_.device->use_srq()) {
  conf_.cap.max_recv_wr = 0;    // QP 无独立接收队列
  conf_.cap.max_recv_sge = 0;
  conf_.cq_size = max(max_send_wr + srq_watermark, cq_size);
} else {
  conf_.cap.max_recv_wr = max(max_recv_wr, recv_buffer_cnt);
  conf_.cq_size = max(max_send_wr + max_recv_wr, cq_size);
}
```

### QP 创建 (`init_qp()`)

```cpp
if (state_->use_srq_) {
  state_->device_->ensure_srq();  // 确保 SRQ + init() buffer
}
// 创建 QP 时：
if (state_->use_srq_) {
  qp_init_attr.srq = device->get_srq_manager()->srq();  // 挂载 SRQ
  qp_init_attr.cap.max_recv_wr = 0;
  qp_init_attr.cap.max_recv_sge = 0;
}
// CUDA/mlx5 路径还需：qp_init_attr_ex.comp_mask |= IBV_QP_INIT_ATTR_SRQ;
```

### `wr_id` 编码约定

| wr_id 值 | 含义 |
|----------|------|
| 0 | 非 SRQ 接收完成 |
| 1 | 发送完成 |
| >1 | SRQ entry 指针 (`reinterpret_cast<ib_srq_recv_entry_t*>(wr_id)`) |

### `poll_completion()` 中的 SRQ 处理

**正常接收**：
```cpp
if (use_srq_) {
  device_->get_srq_manager()->dec_posted_count();  // 可能触发扩容
  srq_entry_ = reinterpret_cast<ib_srq_recv_entry_t*>(wc.wr_id);
  recv_buffer = std::move(srq_entry_->buffer);
}
```

**错误**：
```cpp
if (use_srq_ && wc.wr_id != 1 && wc.wr_id != 0) {
  device_->get_srq_manager()->dec_posted_count();
  device_->get_srq_manager()->repost(entry);  // 重投，不换 buffer
}
```

### `consume()` — 消费后归还

```cpp
if (state_->use_srq_) {
  state_->device_->get_srq_manager()->replenish(state_->srq_entry_, std::move(state_->recv_buf_));
  state_->srq_entry_ = nullptr;
} else {
  state_->add_recv_buffer(std::move(state_->recv_buf_), ...);  // 重投到 QP 接收队列
}
```

### connect/accept 中的差异

- SRQ 模式：调 `on_qp_created()`，**不投递** per-QP 接收 buffer
- 非 SRQ 模式：投 `recv_buffer_cnt` 个 buffer 到 QP 接收队列

---

## 5. 动态扩缩容流程图

```
初始化：
  device 构造 → ibv_create_srq（probe，不分配 buffer）
  QP 创建 → ensure_srq() → init() → 分配 32 块 buffer，post 到 SRQ

运行时：
  收到数据 → poll_completion()
    → dec_posted_count()  // posted--
    → if posted < total/3: expand(total/3)  // 扩容

  消费数据 → consume()
    → replenish(entry, buf)
    → if posted > total*2/3 && total > 32: 缩容（不重投，归还 buffer）
    → else: 重投 buffer 到 SRQ, posted++
    → if posted < total/3: expand(total/3)  // 扩容

空闲回收：
  最后 QP 销毁 → on_qp_destroyed() → active_qp_count_ == 0
    → srq_destruction_lazy() → sleep(3000ms)
    → if active_qp_count_ 仍为 0: srq_manager_.reset()  // 销毁 SRQ
  新 QP 创建 → ensure_srq() → 重建 SRQ + init()
```

## 6. 默认配置

| 配置 | 默认值 | 说明 |
|------|--------|------|
| `use_srq` | `true` | 自动探测，失败回退 per-QP |
| `max_srq_buffer_memory` | 256MB | SRQ buffer 总内存上限 |
| `srq_max_wr` | 4096 | SRQ 队列深度 |
| `srq_buffer_block` | 32 | 初始/最小 buffer 数 |
| `srq_idle_timeout_ms` | 3000 | 空闲销毁延迟 |
| `buffer_size` | 256KB | 每块 buffer 大小 |

**默认初始内存**：32 × 256KB = 8MB
**最大内存**：1024 × 256KB = 256MB

## 7. 线程安全

- `posted_count_` / `total_count_`：`alignas(64)` atomic，多 QP 线程并发安全
- `find_free_slot()`：CAS 无锁分配
- `expand()`：CAS on `total_count_` 防止超 watermark
- `replenish()` 缩容：CAS on `total_count_` 防止低于 `buffer_block_`
- `init()`：CAS on `initialized_` 幂等
- `srq_enabled_`：`bool`，构造后不可变
- `ibv_post_srq_recv`：libibverbs 内部线程安全
