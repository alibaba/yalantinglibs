/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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
#ifndef ASYNC_SIMPLE_CORO_FUTURE_AWAITER_H
#define ASYNC_SIMPLE_CORO_FUTURE_AWAITER_H

#include <atomic>
#include <memory>
#include "async_simple/Signal.h"
#ifndef ASYNC_SIMPLE_USE_MODULES
#include "async_simple/Future.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/experimental/coroutine.h"

#include <type_traits>

#endif  // ASYNC_SIMPLE_USE_MODULES

namespace async_simple {

namespace coro::detail {
template <typename T>
struct FutureAwaiter {
    Future<T> future_;
    async_simple::Slot* slot_ = nullptr;

    bool await_ready() { return future_.hasResult(); }

    template <typename PromiseType>
    bool await_suspend(CoroHandle<PromiseType> continuation) {
        static_assert(std::is_base_of_v<LazyPromiseBase, PromiseType>,
                      "FutureAwaiter is only allowed to be called by Lazy");
        Executor* ex = continuation.promise()._executor;
        Executor::Context ctx = Executor::NULLCTX;
        if (ex != nullptr) {
            ctx = ex->checkout();
        }
        if (continuation.promise()._lazy_local) {
            slot_ = continuation.promise()._lazy_local->getSlot();
        }
        auto state=std::make_shared<std::atomic<bool>>(true);
        if (!signalHelper{Terminate}.tryEmplace(
                slot_, [continuation, ex, ctx, state ](
                           SignalType, Signal*) mutable {
                  if (!state->exchange(false)) {
                    return;
                  }
                  if (ex != nullptr) {
                    ex->checkin(continuation, ctx);
                  }
                  else {
                    continuation.resume();
                  }
                })) {
          return false;
        }
        future_.setContinuation([continuation, ex, ctx,state=std::move(state)](Try<T>&& t) mutable {
            if (!state->exchange(false)) {
                return;
            }
            if (ex != nullptr) {
                ex->checkin(continuation, ctx);
            } else {
                continuation.resume();
            }
        });
        return true;
    }
    auto await_resume() {
        signalHelper{Terminate}.checkHasCanceled(
                slot_, "async_simple::Future is canceled!");
        if constexpr (!std::is_same_v<T, void>)
            return std::move(future_.value());
    }
};
}  // namespace coro::detail

template <typename T>
auto operator co_await(Future<T>&& future) {
    return coro::detail::FutureAwaiter<T>{std::move(future)};
}

template <typename T>
[[deprecated("Require an rvalue future.")]]
auto operator co_await(T&& future) requires IsFuture<std::decay_t<T>>::value {
    return std::move(operator co_await(std::move(future)));
}

}  // namespace async_simple

#endif
