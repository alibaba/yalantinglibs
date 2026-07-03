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

#include <cassert>
#include <memory>

#include "ylt/coro_io/networkdirect/nd_types.hpp"
#include "ylt/coro_io/networkdirect/nd_error.hpp"
#include "nd_device_impl.hpp"

namespace coro_io::detail::verbs_ops {

// post send
inline result_type post_send(native_qp_t* qp, void* request_context,
                             native_sge_t* sge_list, size_type sge_count,
                             int flag, asio::error_code& ec) {
  assert(qp);
  assert(sge_list);
  auto const hr = qp->Send(request_context, sge_list, sge_count, flag);
  ec = static_cast<nd_errc>(hr);
  return hr;
}

// poset recv
inline result_type post_recv(native_qp_t* qp, void* request_context,
                             native_sge_t* sge_list, size_type sge_count,
                             asio::error_code& ec) {
  assert(qp);
  assert(sge_list);
  auto const hr = qp->Receive(request_context, sge_list, sge_count);
  ec = static_cast<nd_errc>(hr);
  return hr;
}

// post read
inline result_type post_read(native_qp_t* qp, void* request_context,
                             native_sge_t* sge_list, size_type sge_count,
                             uint64_t remote_addr, uint32_t remote_token,
                             int flag, asio::error_code& ec) {
  assert(qp);
  assert(sge_list);
  auto const hr = qp->Read(request_context, sge_list, sge_count, remote_addr,
                           remote_token, flag);
  ec = static_cast<nd_errc>(hr);
  return hr;
}

// post write
inline result_type post_write(native_qp_t* qp, void* request_context,
                              native_sge_t* sge_list, size_type sge_count,
                              uint64_t remote_addr, uint32_t remote_token,
                              int flag, asio::error_code& ec) {
  assert(qp);
  assert(sge_list);
  auto const hr = qp->Write(request_context, sge_list, sge_count, remote_addr,
                            remote_token, flag);
  ec = static_cast<nd_errc>(hr);
  return hr;
}

// simulate ibv_allocate_pd
inline native_pd_t* allocate_pd(native_context_t* context,
                                   asio::error_code& ec) {
  if (!context)
    [[unlikely]] {
    return nullptr;
  }

  unique_handle_t handle{create_overlapped_file(context, ec)};
  if (ec) {
    return nullptr;
  }

  auto new_pd = std::make_unique<native_pd_t>();
#ifndef __cpp_exceptions
  if (!new_pd)
  {
    ec = std::errc::not_enough_memory;
    return nullptr;
  }
#endif

  new_pd->context_ = context;
  new_pd->sync_handle_ = std::move(handle);
  return new_pd.release();
}

/// cq ops
// create cq
inline native_cq_t* create_cq(native_context_t* context, size_type cqe,
                                 native_cq_init_attr const& init_attr,
                                 asio::error_code& ec) {
  assert(context);
  native_cq_t* result{nullptr};
  auto const hr = context->CreateCompletionQueue(
      IID_IND2CompletionQueue, init_attr.overlapped_handle_, cqe,
      init_attr.processor_group_, init_attr.processor_affinity_,
      reinterpret_cast<LPVOID*>(std::addressof(result)));
  ec = static_cast<nd_errc>(hr);
  return result;
}

// notify cq
inline result_type notify_cq(native_cq_t* cq,
                                native_cq_notify_attr const& attr,
                                asio::error_code& ec) {
  assert(cq);
  auto const hr = cq->Notify(attr.type_, attr.op_);
  if (hr != ND_SUCCESS && hr != ND_PENDING) {
    ec = static_cast<nd_errc>(hr);
  } else {
    ec.clear();
  }
  return hr;
}

// poll cq
inline size_type poll_cq(native_cq_t* cq, native_wc_t& wc) {
  assert(cq);
  return cq->GetResults(&wc, 1);
}
inline size_type poll_cq(native_cq_t* cq, native_wc_t* wcs, size_type count) {
  assert(cq);
  assert(wcs || count == 0);
  return cq->GetResults(wcs, count);
}
template <size_t Num>
inline size_type poll_cq(native_cq_t* cq, std::array<native_wc_t, Num>& wcs) {
  return poll_cq(cq, wcs.data(), static_cast<size_type>(Num));
}

/// qp ops
// create qp
inline native_qp_t* create_qp(native_pd_t* pd,
                                 native_qp_init_attr const& qp_init_attr,
                                 asio::error_code& ec) {
  assert(pd && pd->context_);
  native_qp_t* result{nullptr};
  auto const hr = pd->context_->CreateQueuePair(
      IID_IND2QueuePair, qp_init_attr.rcq_, qp_init_attr.icq_,
      qp_init_attr.qp_context_, qp_init_attr.max_recv_wr_,
      qp_init_attr.max_send_wr_, qp_init_attr.max_recv_sge_,
      qp_init_attr.max_send_sge_, qp_init_attr.max_inline_data_,
      reinterpret_cast<LPVOID*>(std::addressof(result)));
  ec = static_cast<nd_errc>(hr);
  return result;
}

/// memory region ops
inline ULONG to_native_access_flag(mr_acccess_flag_t access_flag,
                                      int extra_access_flag) {
  ULONG native_access_flag = ND_MR_FLAG_ALLOW_LOCAL_WRITE |
                             ND_MR_FLAG_ALLOW_REMOTE_WRITE |
                             ND_MR_FLAG_ALLOW_REMOTE_READ |
                             static_cast<ULONG>(extra_access_flag);
  switch (access_flag) {
    case mr_access_local_write:
      native_access_flag = ND_MR_FLAG_ALLOW_LOCAL_WRITE |
                           static_cast<ULONG>(extra_access_flag);
      break;
    case mr_access_remote_read:
      native_access_flag = ND_MR_FLAG_ALLOW_LOCAL_WRITE |
                           ND_MR_FLAG_ALLOW_REMOTE_READ |
                           static_cast<ULONG>(extra_access_flag);
      break;
    case mr_access_remote_write:
      native_access_flag = ND_MR_FLAG_ALLOW_LOCAL_WRITE |
                           ND_MR_FLAG_ALLOW_REMOTE_WRITE |
                           ND_MR_FLAG_ALLOW_REMOTE_READ |
                           static_cast<ULONG>(extra_access_flag);
      break;
  }
  return native_access_flag;
}

// register memory region
inline native_mr_t* reg_mr(native_pd_t* pd, void* addr, size_t length,
                              mr_acccess_flag_t access_flag,
                              int extra_access_flag, asio::error_code& ec) {
  assert(pd && pd->context_);
  nd2_memory_region_ptr result{};
  // create memory region interface
  auto hr = pd->context_->CreateMemoryRegion(
      IID_IND2MemoryRegion, pd->sync_handle_.get(),
      reinterpret_cast<LPVOID*>(result.GetAddressOf()));
  ec = static_cast<nd_errc>(hr);
  if (ec) {
    return nullptr;
  }
  // sync register memory region
  assert(result);
  OVERLAPPED sync_ov{0};
  auto const native_access_flag =
      to_native_access_flag(access_flag, extra_access_flag);
  hr = result->Register(addr, length, native_access_flag, &sync_ov);
  switch (hr) {
    case ND_SUCCESS:
      ec.clear();
      break;
    case ND_PENDING:
      hr = result->GetOverlappedResult(&sync_ov, TRUE);
      [[fallthrough]];
    default:
      ec = static_cast<nd_errc>(hr);
      break;
  }
  if (ec) {
    return nullptr;
  }
  return result.Detach();
}

// deregister memory region
inline result_type dereg_mr(native_mr_t* mr, asio::error_code& ec) {
  assert(mr);
  OVERLAPPED sync_ov{0};
  auto hr = mr->Deregister(&sync_ov);
  switch (hr) {
    case ND_SUCCESS:
      break;
    case ND_PENDING:
      hr = mr->GetOverlappedResult(&sync_ov, TRUE);
      [[fallthrough]];
    default:
      ec = static_cast<nd_errc>(hr);
      break;
  }
  return hr;
}

}  // namespace coro_io::detail::verbs_ops
