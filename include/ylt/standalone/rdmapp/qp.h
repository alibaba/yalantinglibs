#pragma once

#include <infiniband/verbs.h>

#include <atomic>
#include <cstdint>
#include <exception>
#include <optional>
#include <utility>
#include <vector>

#include "cq.h"
#include "detail/serdes.h"
#include "device.h"
#include "pd.h"
#include "srq.h"

namespace rdmapp {

static inline struct ibv_sge fill_local_sge(void *addr, uint32_t lkey,
                                            uint32_t length) {
  struct ibv_sge sge = {};
  sge.addr = reinterpret_cast<uint64_t>(addr);
  sge.length = length;
  sge.lkey = lkey;
  return sge;
}

struct deserialized_qp {
  struct qp_header {
    static constexpr size_t kSerializedSize =
        sizeof(uint16_t) + 3 * sizeof(uint32_t) + sizeof(union ibv_gid);
    uint16_t lid;
    uint32_t qp_num;
    uint32_t sq_psn;
    uint32_t user_data_size;
    union ibv_gid gid;
  } header;
  template <class It>
  static deserialized_qp deserialize(It it) {
    deserialized_qp des_qp;
    detail::deserialize(it, des_qp.header.lid);
    detail::deserialize(it, des_qp.header.qp_num);
    detail::deserialize(it, des_qp.header.sq_psn);
    detail::deserialize(it, des_qp.header.user_data_size);
    detail::deserialize(it, des_qp.header.gid);
    return des_qp;
  }
  std::vector<uint8_t> user_data;
};

class qp;
typedef qp *qp_ptr;

/**
 * @brief This class is an abstraction of an Infiniband Queue Pair.
 *
 */
class qp {
  static inline std::atomic<uint32_t> next_sq_psn{1};
  static uint32_t get_next_sq_psn() { return next_sq_psn.fetch_add(1); }
  struct ibv_qp *qp_;
  struct ibv_srq *raw_srq_;
  uint32_t sq_psn_;
  void (qp::*post_recv_fn)(struct ibv_recv_wr const &recv_wr,
                           struct ibv_recv_wr *&bad_recv_wr) const;

  pd_ptr pd_;
  cq_ptr recv_cq_;
  cq_ptr send_cq_;
  srq_ptr srq_;
  std::vector<uint8_t> user_data_;

  /**
   * @brief Creates a new Queue Pair. The Queue Pair will be in the RESET state.
   *
   */
  void create() {
    struct ibv_qp_init_attr qp_init_attr = {};
    ::bzero(&qp_init_attr, sizeof(qp_init_attr));
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.recv_cq = recv_cq_->cq_;
    qp_init_attr.send_cq = send_cq_->cq_;
    qp_init_attr.cap.max_recv_sge = 1;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_wr = 128;
    qp_init_attr.cap.max_send_wr = 128;
    qp_init_attr.sq_sig_all = 0;
    qp_init_attr.qp_context = this;

    if (srq_ != nullptr) {
      qp_init_attr.srq = srq_->srq_;
      raw_srq_ = srq_->srq_;
      post_recv_fn = &qp::post_recv_srq;
    }
    else {
      post_recv_fn = &qp::post_recv_rq;
    }

    qp_ = ::ibv_create_qp(pd_->pd_, &qp_init_attr);
    check_ptr(qp_, "failed to create qp");
    sq_psn_ = get_next_sq_psn();
    // RDMAPP_LOG_TRACE("created qp %p lid=%u qpn=%u psn=%u",
    //                  reinterpret_cast<void *>(qp_), pd_->device()->lid(),
    //                  qp_->qp_num, sq_psn_);
  }

  /**
   * @brief Initializes the Queue Pair. The Queue Pair will be in the INIT
   * state.
   *
   */
  void init() {
    struct ibv_qp_attr qp_attr = {};
    ::bzero(&qp_attr, sizeof(qp_attr));
    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;
    qp_attr.port_num = pd_->device()->port_num();
    qp_attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ |
                              IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
    try {
      check_rc(::ibv_modify_qp(qp_, &qp_attr,
                               IBV_QP_STATE | IBV_QP_PORT |
                                   IBV_QP_ACCESS_FLAGS | IBV_QP_PKEY_INDEX),
               "failed to transition qp to init state");
    } catch (const std::exception &e) {
      // RDMAPP_LOG_ERROR("%s", e.what());
      qp_ = nullptr;
      destroy();
      throw;
    }
  }

  void destroy() {
    if (qp_ == nullptr) [[unlikely]] {
      return;
    }

    if (auto rc = ::ibv_destroy_qp(qp_); rc != 0) [[unlikely]] {
      // RDMAPP_LOG_ERROR("failed to destroy qp %p: %s",
      //                  reinterpret_cast<void *>(qp_), strerror(errno));
    }
    else {
      // RDMAPP_LOG_TRACE("destroyed qp %p", reinterpret_cast<void *>(qp_));
    }
  }

 public:
  class send_awaitable {
    struct ibv_wc wc_;
    std::coroutine_handle<> h_;
    qp_ptr qp_;
    void *local_addr_;
    size_t local_length_;
    uint32_t lkey_;
    std::exception_ptr exception_;
    void *remote_addr_;
    size_t remote_length_;
    uint32_t rkey_;
    uint64_t compare_add_;
    uint64_t swap_;
    uint32_t imm_;
    const enum ibv_wr_opcode opcode_;

   public:
    send_awaitable(qp_ptr qp, void *local_addr, size_t local_length,
                   uint32_t lkey, enum ibv_wr_opcode opcode)
        : wc_(),
          qp_(qp),
          local_addr_(local_addr),
          local_length_(local_length),
          lkey_(lkey),
          opcode_(opcode) {}

    send_awaitable(qp_ptr qp, void *local_addr, size_t local_length,
                   uint32_t lkey, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr)
        : qp_(qp),
          local_addr_(local_addr),
          local_length_(local_length),
          lkey_(lkey),
          opcode_(opcode),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()) {}

    send_awaitable(qp_ptr qp, void *local_addr, size_t local_length,
                   uint32_t lkey, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr, uint32_t imm)
        : qp_(qp),
          local_addr_(local_addr),
          local_length_(local_length),
          lkey_(lkey),
          opcode_(opcode),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()),
          imm_(imm) {}

    send_awaitable(qp_ptr qp, void *local_addr, size_t local_length,
                   uint32_t lkey, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr, uint64_t add)
        : qp_(qp),
          local_addr_(local_addr),
          local_length_(local_length),
          lkey_(lkey),
          opcode_(opcode),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()),
          compare_add_(add) {}

    send_awaitable(qp_ptr qp, void *local_addr, size_t local_length,
                   uint32_t lkey, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr, uint64_t compare, uint64_t swap)
        : qp_(qp),
          local_addr_(local_addr),
          local_length_(local_length),
          lkey_(lkey),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()),
          compare_add_(compare),
          swap_(swap),
          opcode_(opcode) {}

    send_awaitable(qp_ptr qp, local_mr_ptr local_mr, enum ibv_wr_opcode opcode)
        : wc_(),
          qp_(qp),
          local_addr_(local_mr->addr()),
          local_length_(local_mr->length()),
          lkey_(local_mr->lkey()),
          opcode_(opcode) {}
    send_awaitable(qp_ptr qp, local_mr_ptr local_mr, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr)
        : qp_(qp),
          local_addr_(local_mr->addr()),
          local_length_(local_mr->length()),
          lkey_(local_mr->lkey()),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()),
          opcode_(opcode) {}

    send_awaitable(qp_ptr qp, local_mr_ptr local_mr, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr, uint32_t imm)
        : qp_(qp),
          local_addr_(local_mr->addr()),
          local_length_(local_mr->length()),
          lkey_(local_mr->lkey()),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()),
          imm_(imm),
          opcode_(opcode) {}
    send_awaitable(qp_ptr qp, local_mr_ptr local_mr, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr, uint64_t add)
        : qp_(qp),
          local_addr_(local_mr->addr()),
          local_length_(local_mr->length()),
          lkey_(local_mr->lkey()),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()),
          compare_add_(add),
          opcode_(opcode) {}

    send_awaitable(qp_ptr qp, local_mr_ptr local_mr, enum ibv_wr_opcode opcode,
                   remote_mr const &remote_mr, uint64_t compare, uint64_t swap)
        : qp_(qp),
          local_addr_(local_mr->addr()),
          local_length_(local_mr->length()),
          lkey_(local_mr->lkey()),
          remote_addr_(remote_mr.addr()),
          remote_length_(remote_mr.length()),
          rkey_(remote_mr.rkey()),
          compare_add_(compare),
          swap_(swap),
          opcode_(opcode) {}

    void complete(struct ibv_wc const &wc) {
      wc_ = wc;
      h_.resume();
    }
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> h) noexcept {
      h_ = h;
      auto send_sge = fill_local_sge(local_addr_, lkey_, local_length_);

      struct ibv_send_wr send_wr = {};
      struct ibv_send_wr *bad_send_wr = nullptr;
      send_wr.opcode = opcode_;
      send_wr.next = nullptr;
      send_wr.num_sge = 1;
      send_wr.wr_id = reinterpret_cast<uint64_t>(this);
      send_wr.send_flags = IBV_SEND_SIGNALED;
      send_wr.sg_list = &send_sge;
      if (is_rdma()) {
        assert(remote_addr_ != nullptr);
        send_wr.wr.rdma.remote_addr = reinterpret_cast<uint64_t>(remote_addr_);
        send_wr.wr.rdma.rkey = rkey_;
        if (opcode_ == IBV_WR_RDMA_WRITE_WITH_IMM) {
          send_wr.imm_data = imm_;
        }
      }
      else if (is_atomic()) {
        assert(remote_addr_ != nullptr);
        send_wr.wr.atomic.remote_addr =
            reinterpret_cast<uint64_t>(remote_addr_);
        send_wr.wr.atomic.rkey = rkey_;
        send_wr.wr.atomic.compare_add = compare_add_;
        if (opcode_ == IBV_WR_ATOMIC_CMP_AND_SWP) {
          send_wr.wr.atomic.swap = swap_;
        }
      }

      try {
        qp_->post_send(send_wr, bad_send_wr);
      } catch (std::runtime_error &e) {
        exception_ = std::make_exception_ptr(e);
        return false;
      }
      return true;
    }
    struct ibv_wc await_resume() const {
      if (exception_) [[unlikely]] {
        std::rethrow_exception(exception_);
      }
      check_wc_status(wc_.status, "failed to send");
      return wc_;
    }

    constexpr bool is_rdma() const {
      return opcode_ == IBV_WR_RDMA_READ || opcode_ == IBV_WR_RDMA_WRITE ||
             opcode_ == IBV_WR_RDMA_WRITE_WITH_IMM;
    }

    constexpr bool is_atomic() const {
      return opcode_ == IBV_WR_ATOMIC_CMP_AND_SWP ||
             opcode_ == IBV_WR_ATOMIC_FETCH_AND_ADD;
    }
  };

  class recv_awaitable {
    struct ibv_wc wc_;
    std::coroutine_handle<> h_;
    qp *qp_;
    void *local_addr_;
    size_t local_length_;
    uint32_t lkey_;
    std::exception_ptr exception_;
    enum ibv_wr_opcode opcode_;

   public:
    recv_awaitable(qp_ptr qp, local_mr_ptr local_mr)
        : wc_(),
          qp_(qp),
          local_addr_(local_mr->addr()),
          local_length_(local_mr->length()),
          lkey_(local_mr->lkey()) {}

    recv_awaitable(qp_ptr qp, void *local_addr, size_t local_length,
                   uint32_t lkey)
        : wc_(),
          qp_(qp),
          local_addr_(local_addr),
          local_length_(local_length),
          lkey_(lkey) {}

    void complete(struct ibv_wc const &wc) {
      wc_ = wc;
      h_.resume();
    }

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> h) noexcept {
      h_ = h;
      auto recv_sge = fill_local_sge(local_addr_, lkey_, local_length_);

      struct ibv_recv_wr recv_wr = {};
      struct ibv_recv_wr *bad_recv_wr = nullptr;
      recv_wr.next = nullptr;
      recv_wr.num_sge = 1;
      recv_wr.wr_id = reinterpret_cast<uint64_t>(this);
      recv_wr.sg_list = &recv_sge;

      try {
        qp_->post_recv(recv_wr, bad_recv_wr);
      } catch (std::runtime_error &e) {
        exception_ = std::make_exception_ptr(e);
        return false;
      }
      return true;
    }

    std::pair<uint32_t, std::optional<uint32_t>> await_resume() const {
      if (exception_) [[unlikely]] {
        std::rethrow_exception(exception_);
      }
      check_wc_status(wc_.status, "failed to recv");
      if (wc_.wc_flags & IBV_WC_WITH_IMM) {
        return std::make_pair(wc_.byte_len, wc_.imm_data);
      }
      return std::make_pair(wc_.byte_len, std::nullopt);
    }
  };

  /**
   * @brief Construct a new qp object. The Queue Pair will be created with the
   * given remote Queue Pair parameters. Once constructed, the Queue Pair will
   * be in the RTS state.
   *
   * @param remote_lid The LID of the remote Queue Pair.
   * @param remote_qpn The QPN of the remote Queue Pair.
   * @param remote_psn The PSN of the remote Queue Pair.
   * @param pd The protection domain of the new Queue Pair.
   * @param cq The completion queue of both send and recv work completions.
   * @param srq (Optional) If set, all recv work requests will be posted to this
   * SRQ.
   */
  qp(const uint16_t remote_lid, const uint32_t remote_qpn,
     const uint32_t remote_psn, const union ibv_gid remote_gid, pd_ptr pd,
     cq_ptr cq, srq_ptr srq = nullptr)
      : qp(remote_lid, remote_qpn, remote_psn, remote_gid, pd, cq, cq, srq) {}

  /**
   * @brief Construct a new qp object. The Queue Pair will be created with the
   * given remote Queue Pair parameters. Once constructed, the Queue Pair will
   * be in the RTS state.
   *
   * @param remote_lid The LID of the remote Queue Pair.
   * @param remote_qpn The QPN of the remote Queue Pair.
   * @param remote_psn The PSN of the remote Queue Pair.
   * @param pd The protection domain of the new Queue Pair.
   * @param recv_cq The completion queue of recv work completions.
   * @param send_cq The completion queue of send work completions.
   * @param srq (Optional) If set, all recv work requests will be posted to this
   * SRQ.
   */
  qp(const uint16_t remote_lid, const uint32_t remote_qpn,
     const uint32_t remote_psn, const union ibv_gid remote_gid, pd_ptr pd,
     cq_ptr recv_cq, cq_ptr send_cq, srq_ptr srq = nullptr)
      : qp(pd, recv_cq, send_cq, srq) {
    rtr(remote_lid, remote_qpn, remote_psn, remote_gid);
    rts();
  }

  /**
   * @brief Construct a new qp object. The constructed Queue Pair will be in
   * INIT state.
   *
   * @param pd The protection domain of the new Queue Pair.
   * @param cq The completion queue of both send and recv work completions.
   * @param srq (Optional) If set, all recv work requests will be posted to this
   * SRQ.
   */
  qp(pd_ptr pd, cq_ptr cq, srq_ptr srq = nullptr) : qp(pd, cq, cq, srq) {}

  /**
   * @brief Construct a new qp object. The constructed Queue Pair will be in
   * INIT state.
   *
   * @param pd The protection domain of the new Queue Pair.
   * @param recv_cq The completion queue of recv work completions.
   * @param send_cq The completion queue of send work completions.
   * @param srq (Optional) If set, all recv work requests will be posted to this
   * SRQ.
   */
  qp(pd_ptr pd, cq_ptr recv_cq, cq_ptr send_cq, srq_ptr srq = nullptr)
      : qp_(nullptr), pd_(pd), recv_cq_(recv_cq), send_cq_(send_cq), srq_(srq) {
    create();
    init();
  }

  /**
   * @brief This function is used to post a send work request to the Queue Pair.
   *
   * @param recv_wr The work request to post.
   * @param bad_recv_wr A pointer to a work request that will be set to the
   * first work request that failed to post.
   */
  void post_send(struct ibv_send_wr const &send_wr,
                 struct ibv_send_wr *&bad_send_wr) {
    // RDMAPP_LOG_TRACE("post send wr_id=%p addr=%p",
    //                  reinterpret_cast<void *>(send_wr.wr_id),
    //                  reinterpret_cast<void *>(send_wr.sg_list->addr));
    check_rc(::ibv_post_send(qp_, const_cast<struct ibv_send_wr *>(&send_wr),
                             &bad_send_wr),
             "failed to post send");
  }

  /**
   * @brief This function is used to post a recv work request to the Queue Pair.
   * It will be posted to either RQ or SRQ depending on whether or not SRQ is
   * set.
   *
   * @param recv_wr The work request to post.
   * @param bad_recv_wr A pointer to a work request that will be set to the
   * first work request that failed to post.
   */
  void post_recv(struct ibv_recv_wr const &recv_wr,
                 struct ibv_recv_wr *&bad_recv_wr) const {
    (this->*(post_recv_fn))(recv_wr, bad_recv_wr);
  }

  /**
   * @brief This function sends a registered local memory region to remote.
   *
   * @param local_mr Registered local memory region, whose lifetime is
   * controlled by a smart pointer.
   * @return send_awaitable A coroutine returning length of the data sent.
   */
  [[nodiscard]] send_awaitable send(local_mr_ptr local_mr) {
    return qp::send_awaitable(this, local_mr, ibv_wr_opcode::IBV_WR_SEND);
  }

  /**
   * @brief This function writes a registered local memory region to remote.
   *
   * @param remote_mr Remote memory region handle.
   * @param local_mr Registered local memory region, whose lifetime is
   * controlled by a smart pointer.
   * @return send_awaitable A coroutine returning length of the data written.
   */
  [[nodiscard]] send_awaitable write(remote_mr const &remote_mr,
                                     local_mr_ptr local_mr) {
    return qp::send_awaitable(this, local_mr, IBV_WR_RDMA_WRITE, remote_mr);
  }

  /**
   * @brief This function writes a registered local memory region to remote with
   * an immediate value.
   *
   * @param remote_mr Remote memory region handle.
   * @param local_mr Registered local memory region, whose lifetime is
   * controlled by a smart pointer.
   * @param imm The immediate value.
   * @return send_awaitable A coroutine returning length of the data sent.
   */
  [[nodiscard]] send_awaitable write_with_imm(remote_mr const &remote_mr,
                                              local_mr_ptr local_mr,
                                              uint32_t imm) {
    return qp::send_awaitable(this, local_mr, IBV_WR_RDMA_WRITE_WITH_IMM,
                              remote_mr, imm);
  }

  /**
   * @brief This function reads to local memory region from remote.
   *
   * @param remote_mr Remote memory region handle.
   * @param local_mr Registered local memory region, whose lifetime is
   * controlled by a smart pointer.
   * @return send_awaitable A coroutine returning length of the data read.
   */
  [[nodiscard]] send_awaitable read(remote_mr const &remote_mr,
                                    local_mr_ptr local_mr) {
    return qp::send_awaitable(this, local_mr, IBV_WR_RDMA_READ, remote_mr);
  }

  /**
   * @brief This function performs an atomic fetch-and-add operation on the
   * given remote memory region.
   *
   * @param remote_mr Remote memory region handle.
   * @param local_mr Registered local memory region, whose lifetime is
   * controlled by a smart pointer.
   * @param add The delta.
   * @return send_awaitable A coroutine returning length of the data sent.
   */
  [[nodiscard]] send_awaitable fetch_and_add(remote_mr const &remote_mr,
                                             local_mr_ptr local_mr,
                                             uint64_t add) {
    assert(pd_->device()->is_fetch_and_add_supported());
    return qp::send_awaitable(this, local_mr, IBV_WR_ATOMIC_FETCH_AND_ADD,
                              remote_mr, add);
  }

  /**
   * @brief This function performs an atomic compare-and-swap operation on the
   * given remote memory region.
   *
   * @param remote_mr Remote memory region handle.
   * @param local_mr Registered local memory region, whose lifetime is
   * controlled by a smart pointer.
   * @param compare The expected old value.
   * @param swap The desired new value.
   * @return send_awaitable A coroutine returning length of the data sent.
   */
  [[nodiscard]] send_awaitable compare_and_swap(remote_mr const &remote_mr,
                                                local_mr_ptr local_mr,
                                                uint64_t compare,
                                                uint64_t swap) {
    assert(pd_->device()->is_compare_and_swap_supported());
    return qp::send_awaitable(this, local_mr, IBV_WR_ATOMIC_CMP_AND_SWP,
                              remote_mr, compare, swap);
  }

  /**
   * @brief This function posts a recv request on the queue pair. The buffer
   * will be filled with data received.
   *
   * @param local_mr Registered local memory region, whose lifetime is
   * controlled by a smart pointer.
   * @return recv_awaitable A coroutine returning std::pair<uint32_t,
   * std::optional<uint32_t>>, with first indicating the length of received
   * data, and second indicating the immediate value if any.
   */
  [[nodiscard]] recv_awaitable recv(local_mr_ptr local_mr) {
    return qp::recv_awaitable(this, local_mr);
  }

  /**
   * @brief This function serializes a Queue Pair prepared to be sent to a
   * buffer.
   *
   * @return std::vector<uint8_t> The serialized QP.
   */
  std::vector<uint8_t> serialize() const {
    std::vector<uint8_t> buffer;
    auto it = std::back_inserter(buffer);
    detail::serialize(pd_->device()->lid(), it);
    detail::serialize(qp_->qp_num, it);
    detail::serialize(sq_psn_, it);
    detail::serialize(static_cast<uint32_t>(user_data_.size()), it);
    detail::serialize(pd_->device()->gid(), it);
    std::copy(user_data_.cbegin(), user_data_.cend(), it);
    return buffer;
  }

  /**
   * @brief This function provides access to the extra user data of the Queue
   * Pair.
   *
   * @return std::vector<uint8_t>& The extra user data.
   */
  std::vector<uint8_t> &user_data() { return user_data_; }

  /**
   * @brief This function provides access to the Protection Domain of the Queue
   * Pair.
   *
   * @return pd_ptr Pointer to the PD.
   */
  pd_ptr pd() const { return pd_; }

  ~qp() { destroy(); }

  /**
   * @brief This function transitions the Queue Pair to the RTR state.
   *
   * @param remote_lid The remote LID.
   * @param remote_qpn The remote QPN.
   * @param remote_psn The remote PSN.
   * @param remote_gid The remote GID.
   */
  void rtr(uint16_t remote_lid, uint32_t remote_qpn, uint32_t remote_psn,
           union ibv_gid remote_gid) {
    struct ibv_qp_attr qp_attr = {};
    ::bzero(&qp_attr, sizeof(qp_attr));
    qp_attr.qp_state = IBV_QPS_RTR;
    qp_attr.path_mtu = IBV_MTU_4096;
    qp_attr.dest_qp_num = remote_qpn;
    qp_attr.rq_psn = remote_psn;
    qp_attr.max_dest_rd_atomic = 16;
    qp_attr.min_rnr_timer = 12;
    qp_attr.ah_attr.is_global = 1;
    qp_attr.ah_attr.grh.dgid = remote_gid;
    qp_attr.ah_attr.grh.sgid_index = pd_->device_->gid_index_;
    qp_attr.ah_attr.grh.hop_limit = 16;
    qp_attr.ah_attr.dlid = remote_lid;
    qp_attr.ah_attr.sl = 0;
    qp_attr.ah_attr.src_path_bits = 0;
    qp_attr.ah_attr.port_num = pd_->device()->port_num();

    try {
      check_rc(
          ::ibv_modify_qp(qp_, &qp_attr,
                          IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
                              IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                              IBV_QP_MIN_RNR_TIMER | IBV_QP_MAX_DEST_RD_ATOMIC),
          "failed to transition qp to rtr state");
    } catch (const std::exception &e) {
      // RDMAPP_LOG_ERROR("%s", e.what());
      qp_ = nullptr;
      destroy();
      throw;
    }
  }

  /**
   * @brief This function transitions the Queue Pair to the RTS state.
   *
   */
  void rts() {
    struct ibv_qp_attr qp_attr = {};
    ::bzero(&qp_attr, sizeof(qp_attr));
    qp_attr.qp_state = IBV_QPS_RTS;
    qp_attr.timeout = 14;
    qp_attr.retry_cnt = 7;
    qp_attr.rnr_retry = 7;
    qp_attr.max_rd_atomic = 16;
    qp_attr.sq_psn = sq_psn_;

    try {
      check_rc(::ibv_modify_qp(qp_, &qp_attr,
                               IBV_QP_STATE | IBV_QP_TIMEOUT |
                                   IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY |
                                   IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC),
               "failed to transition qp to rts state");
    } catch (std::exception const &e) {
      // RDMAPP_LOG_ERROR("%s", e.what());
      qp_ = nullptr;
      destroy();
      throw;
    }
  }

 private:
  /**
   * @brief This function posts a recv request on the Queue Pair's own RQ.
   *
   * @param recv_wr
   * @param bad_recv_wr
   */
  void post_recv_rq(struct ibv_recv_wr const &recv_wr,
                    struct ibv_recv_wr *&bad_recv_wr) const {
    // RDMAPP_LOG_TRACE("post recv wr_id=%p addr=%p",
    //                  reinterpret_cast<void *>(recv_wr.wr_id),
    //                  reinterpret_cast<void *>(recv_wr.sg_list->addr));
    check_rc(::ibv_post_recv(qp_, const_cast<struct ibv_recv_wr *>(&recv_wr),
                             &bad_recv_wr),
             "failed to post recv");
  }

  /**
   * @brief This function posts a send request on the Queue Pair's SRQ.
   *
   * @param recv_wr
   * @param bad_recv_wr
   */
  void post_recv_srq(struct ibv_recv_wr const &recv_wr,
                     struct ibv_recv_wr *&bad_recv_wr) const {
    check_rc(
        ::ibv_post_srq_recv(
            raw_srq_, const_cast<struct ibv_recv_wr *>(&recv_wr), &bad_recv_wr),
        "failed to post srq recv");
  }
};

static inline void process_wc(struct ibv_wc const &wc) {
  if (wc.opcode & IBV_WC_RECV) {
    auto &recv_awaitable =
        *reinterpret_cast<rdmapp::qp::recv_awaitable *>(wc.wr_id);
    recv_awaitable.complete(wc);
  }
  else {
    auto &send_awaitable =
        *reinterpret_cast<rdmapp::qp::send_awaitable *>(wc.wr_id);
    send_awaitable.complete(wc);
  }
}

}  // namespace rdmapp
