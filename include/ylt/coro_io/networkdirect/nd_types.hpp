/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
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

#include <array>
#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

#include <guiddef.h>
#include <libloaderapi.h>
#include <winnt.h>
#include <wrl/client.h>
#include <ws2spi.h>

#include "networkdirect.hpp"
#include "nd_commons.hpp"

namespace coro_io {

using size_type = ULONG;
using result_type = HRESULT;

}  // namespace coro_io

#include "detail/nd_impl_types.hpp"
