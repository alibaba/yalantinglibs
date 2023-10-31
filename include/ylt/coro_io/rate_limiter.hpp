/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/Mutex.h>
#include <async_simple/coro/Sleep.h>
#include <async_simple/coro/SyncAwait.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <ylt/easylog.hpp>

namespace coro_io {
class RateLimiter {
 public:
  async_simple::coro::Lazy<double> acquire(int permits) {
    co_await this->mutex_.coLock();
    long wait_mills = reserveAndGetWaitLength(permits, currentTimeMills());
    this->mutex_.unlock();
    co_await async_simple::coro::sleep(std::chrono::milliseconds(wait_mills));
    co_return 1.0 * wait_mills / 1000;
  }
  async_simple::coro::Lazy<void> setRate(double permitsPerSecond) {
    co_await this->mutex_.coLock();
    doSetRate(permitsPerSecond, currentTimeMills());
    this->mutex_.unlock();
  }

 protected:
  virtual void doSetRate(double permitsPerSecond, long now_micros) = 0;
  virtual long reserveEarliestAvailable(int permits, long now_micros) = 0;
  long currentTimeMills() {
    std::chrono::system_clock::time_point now =
        std::chrono::system_clock::now();
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch());
    return ms.count();
  }

 private:
  long reserveAndGetWaitLength(int permits, long now_micros) {
    long moment_available = reserveEarliestAvailable(permits, now_micros);
    return std::max(moment_available - now_micros, 0L);
  }

  async_simple::coro::Mutex mutex_;
};

class AbstractSmoothRateLimiter : public RateLimiter {
 protected:
  virtual void doSetRate(double permits_per_second,
                         double stable_internal_micros) = 0;
  virtual long storedPermitsToWaitTime(double stored_permits,
                                       double permits_to_take) = 0;
  virtual double coolDownInternalMicros() = 0;
  void resync(long now_micros) {
    // if next_free_ticket is in the past, resync to now
    ELOG_DEBUG << "now micros: " << now_micros << ", next_free_ticket_micros_: "
               << this->next_free_ticket_micros_;
    if (now_micros > this->next_free_ticket_micros_) {
      double newPermits = (now_micros - this->next_free_ticket_micros_) /
                          coolDownInternalMicros();
      this->stored_permits_ =
          std::min(this->max_permits_, this->stored_permits_ + newPermits);
      this->next_free_ticket_micros_ = now_micros;
    }
  }
  void doSetRate(double permits_per_second, long now_micros) override {
    resync(now_micros);
    double stable_internal_micros = 1000 / permits_per_second;
    this->stable_internal_micros_ = stable_internal_micros;
    doSetRate(permits_per_second, stable_internal_micros);
  }
  long reserveEarliestAvailable(int required_permits, long now_micros) {
    resync(now_micros);
    long return_value = this->next_free_ticket_micros_;
    double stored_permits_to_spend =
        std::min((double)required_permits, this->stored_permits_);
    double fresh_permits = required_permits - stored_permits_to_spend;
    long wait_micros = storedPermitsToWaitTime(this->stored_permits_,
                                               stored_permits_to_spend) +
                       (long)(fresh_permits * this->stable_internal_micros_);
    this->next_free_ticket_micros_ += wait_micros;
    this->stored_permits_ -= stored_permits_to_spend;
    return return_value;
  }

  /**
   * The currently stored permits.
   */
  double stored_permits_ = 0;
  /**
   * The maximum number of stored permits.
   */
  double max_permits_ = 0;
  /**
   * The interval between two unit requests, at our stable rate. E.g., a stable
   * rate of 5 permits per second has a stable internal of 200ms.
   */
  double stable_internal_micros_ = 0;
  /**
   * The time when the next request (no matter its size) will be granted. After
   * granting a request, this is pushed further in the future. Large requests
   * push this further than small requests.
   */
  long next_free_ticket_micros_ = 0;
};

class SmoothBurstyRateLimiter : public AbstractSmoothRateLimiter {
 public:
  SmoothBurstyRateLimiter(double permits_per_second) {
    this->max_burst_seconds_ = 1.0;
    async_simple::coro::syncAwait(setRate(permits_per_second));
  }

 protected:
  void doSetRate(double permits_per_second, double stable_internal_micros) {
    double old_max_permits = this->max_permits_;
    this->max_permits_ = this->max_burst_seconds_ * permits_per_second;
    this->stored_permits_ =
        (0 == old_max_permits)
            ? 0
            : this->stored_permits_ * this->max_permits_ / old_max_permits;
    ELOG_DEBUG << "max_permits_: " << this->max_permits_
               << ", stored_permits_:" << this->stored_permits_;
  }

  long storedPermitsToWaitTime(double stored_permits, double permits_to_take) {
    return 0L;
  }

  double coolDownInternalMicros() { return this->stable_internal_micros_; }

 private:
  /**
   * The work(permits) of how many seconds can be saved up if the RateLimiter is
   * unused.
   */
  double max_burst_seconds_ = 0;
};
}  // namespace coro_io