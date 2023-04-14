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
#pragma once

#include "reflection.hpp"

namespace struct_pack {
template <detail::trivial_serializable T>
struct trivial_view {
 private:
  const T* ref;

 public:
  trivial_view(const T& t) : ref(&t){};
  trivial_view(const trivial_view&) = default;
  trivial_view(trivial_view&&) = default;
  trivial_view() : ref(nullptr){};

  trivial_view& operator=(const trivial_view&) = default;
  trivial_view& operator=(trivial_view&&) = default;

  using value_type = T;

  const T& get() const {
    assert(ref != nullptr);
    return *ref;
  }
  const T* operator->() const {
    assert(ref != nullptr);
    return ref;
  }
};
}  // namespace struct_pack
