#pragma once

#include "detail/own_ring_driver.hpp"

namespace coro_io {

using OwnRingIoState = detail::OwnRingDriver;

class OwnRingIoContext {
 public:
  using Mode = OwnRingIoState::Mode;
  using Options = OwnRingIoState::Options;
  using ReadRequest = OwnRingIoState::ReadRequest;
  using Statistics = OwnRingIoState::Statistics;
#ifdef OWN_RING_FAULT_INJECT
  using FaultHooks = OwnRingIoState::FaultHooks;
  static FaultHooks &fault() { return OwnRingIoState::fault(); }
#endif

  explicit OwnRingIoContext(unsigned entries = 4096)
      : state_(std::make_shared<OwnRingIoState>(entries)) {}
  explicit OwnRingIoContext(Options options)
      : state_(std::make_shared<OwnRingIoState>(options)) {}
  OwnRingIoContext(const OwnRingIoContext &) = delete;
  OwnRingIoContext &operator=(const OwnRingIoContext &) = delete;
  ~OwnRingIoContext() {
    if (state_->on_owner_thread()) {
      std::terminate();
    }
    state_->stop();
  }

  auto share_state() const noexcept { return state_; }
  bool ok() const noexcept { return state_->ok(); }
  int init_error() const noexcept { return state_->init_error(); }
  unsigned setup_flags() const noexcept { return state_->setup_flags(); }
  bool defer_enabled() const noexcept { return state_->defer_enabled(); }
  bool post_resume() const noexcept { return state_->post_resume(); }
  Statistics statistics() const noexcept { return state_->statistics(); }
  uint64_t enter_count() const noexcept { return state_->enter_count(); }
  uint64_t completed_count() const noexcept {
    return state_->completed_count();
  }
  size_t inflight_size() const noexcept { return state_->inflight_size(); }
  void request_stop() noexcept { state_->request_stop(); }
  void stop() noexcept { state_->stop(); }
  void submit_read(ReadRequest request) noexcept {
    state_->submit_read(std::move(request));
  }
  bool submit_borrowed_read(ReadRequest &request) noexcept {
    return state_->submit_borrowed_read(request);
  }

 private:
  std::shared_ptr<OwnRingIoState> state_;
};

}  // namespace coro_io

#include "detail/own_ring_context_pool.hpp"
