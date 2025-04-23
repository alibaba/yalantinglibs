/*
 * Copyright (c) 2025;
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
#ifndef ASYNC_SIMPLE_CORO_ASYNC_NO_AWAIT_H
#define ASYNC_SIMPLE_CORO_ASYNC_NO_AWAIT_H

#ifndef ASYNC_SIMPLE_USE_MODULES
#include "async_simple/Try.h"
#include "async_simple/Executor.h"
#include <functional>
#include <string>
#endif  // ASYNC_SIMPLE_USE_MODULES

namespace async_simple {
namespace coro {

template <typename T>
class Lazy;

// Async await on a coroutine, non-blocking, with two callbacks for success and failure.
// - Success callback receives the coroutine's result (ValueType).
// - Failure callback receives an error message from the exception.
template <typename LazyType>
inline void asyncNoAwait(
    LazyType&& lazy,
    std::function<void(typename std::decay_t<LazyType>::ValueType)> success,
    std::function<void(std::string)> fail) {
    using ValueType = typename std::decay_t<LazyType>::ValueType;
    std::move(std::forward<LazyType>(lazy))
        .start([success, fail](Try<ValueType> result) {
            if (result.hasError()) {
                try {
                    std::rethrow_exception(result.getException());
                } catch (const std::exception& e) {
                    if (fail) fail("Exception: " + std::string(e.what()));
                } catch (...) {
                    if (fail) fail("Exception: Unknown error");
                }
            } else {
                if (success) success(result.value());
            }
        });
}

// Overload with executor, similar to syncAwait(LazyType, Executor*).
template <typename LazyType>
inline void asyncNoAwait(
    LazyType&& lazy,
    Executor* ex,
    std::function<void(typename std::decay_t<LazyType>::ValueType)> success,
    std::function<void(std::string)> fail) {
    asyncNoAwait(std::move(std::forward<LazyType>(lazy)).via(ex), success, fail);
}

}  // namespace coro
}  // namespace async_simple

#endif  // ASYNC_SIMPLE_CORO_ASYNC_NO_AWAIT_H