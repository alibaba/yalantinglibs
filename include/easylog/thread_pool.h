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

#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

namespace easylog {

template <typename T>
class blocking_queue {
 public:
  blocking_queue() = default;

  void enqueue(T &&item) {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_.push_back(std::move(item));
    }
    push_cv_.notify_one();
  }

  bool dequeue_for(T &popped_item, std::chrono::milliseconds wait_duration) {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (!push_cv_.wait_for(lock, wait_duration, [this] {
            return !queue_.empty();
          })) {
        return false;
      }

      popped_item = std::move(queue_.front());
      queue_.pop_front();
    }
    return true;
  }

  size_t size() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return queue_.size();
  }

 private:
  std::mutex queue_mutex_;
  std::condition_variable push_cv_;
  std::list<T> queue_;
};

class thread_pool {
 public:
  static thread_pool &instance() {
    static thread_pool thread_instance;
    return thread_instance;
  }

 public:
  typedef std::function<bool()> Task;
  using item_type = std::unique_ptr<Task>;

  void init(size_t threads_n) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!threads_.empty()) {
      return;
    }

    for (size_t i = 0; i < threads_n; i++) {
      threads_.emplace_back([this] {
        thread_pool::worker_loop();
      });
    }
  }

  void async(Task &&task) {
    queue_.enqueue(std::make_unique<Task>(std::move(task)));
  }

 private:
  thread_pool() = default;
  thread_pool(const thread_pool &) = default;

  ~thread_pool() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (threads_.empty()) {
      return;
    }

    for (size_t i = 0; i < threads_.size(); i++) {
      async([]() {
        return false;
      });
    }

    for (auto &t : threads_) {
      t.join();
    }

    threads_.clear();
  }

  blocking_queue<item_type> queue_;

  std::vector<std::thread> threads_;

  std::mutex mutex_;

 private:
  void worker_loop() {
    while (process_next_msg()) {
    }
  }

  bool process_next_msg() {
    std::unique_ptr<Task> record_task;
    bool dequeued = queue_.dequeue_for(record_task, std::chrono::seconds(10));
    if (!dequeued) {
      return true;
    }

    return (*record_task)();
  }
};

}  // namespace easylog
