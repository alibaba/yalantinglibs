//
// detail/lib_aio_descriptor_write_at_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//added by ysbarney

#ifndef ASIO_DETAIL_LIB_AIO_DESCRIPTOR_WRITE_AT_OP_HPP
#define ASIO_DETAIL_LIB_AIO_DESCRIPTOR_WRITE_AT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_LIB_AIO)
#include <libaio.h>
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/descriptor_ops.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/lib_aio_io_operation.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/handler_work.hpp"
#include "asio/detail/reactor_op.hpp"
#include "asio/detail/memory.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {
  
template <typename ConstBufferSequence>
class lib_aio_descriptor_write_at_op_base : public reactor_op
{
public:
  lib_aio_descriptor_write_at_op_base(
      const asio::error_code& success_ec, int descriptor,
      descriptor_ops::state_type state, int event_fd, io_context_t* aio_ctx, 
      func_type complete_func)
    : reactor_op(success_ec,
        &lib_aio_descriptor_write_at_op_base::do_perform, complete_func),
      event_fd_(event_fd),
      aio_ctx_(aio_ctx),
      descriptor_(descriptor),
      state_(state)
  {
  }

  static status do_perform(reactor_op* base)
  {
    lib_aio_descriptor_write_at_op_base* o(
        static_cast<lib_aio_descriptor_write_at_op_base*>(base));
    
    uint64_t finished_aio=0, r=0;
    asio::error_code ec;
    std::size_t ret = descriptor_ops::sync_read1(o->event_fd_, descriptor_ops::non_blocking,
            &finished_aio, sizeof(uint64_t), ec);
    if(!ret)
    {
      return not_done;
    }

    // aio events;
    int batch_size = lib_aio_io_operation::batch_size;
    struct io_event aio_events[batch_size];
    while(finished_aio > 0)
    {
      r = ::io_getevents(*(o->aio_ctx_), 0, batch_size, aio_events, NULL);
      for(int i=0; i < r; i++)
      {
        if(aio_events[i].res2 != 0) 
        {
          return not_done;
        }
        o->bytes_transferred_ += aio_events[i].res;
      }

      finished_aio -= r;
    }

    return done;
  }

public:
  int event_fd_;
  io_context_t* aio_ctx_;

private:
  int descriptor_;
  descriptor_ops::state_type state_;
};

template <typename ConstBufferSequence, typename Handler, typename IoExecutor>
class lib_aio_descriptor_write_at_op
  : public lib_aio_descriptor_write_at_op_base<ConstBufferSequence>
{
public:
  typedef Handler handler_type;
  typedef IoExecutor io_executor_type;

  ASIO_DEFINE_HANDLER_PTR(lib_aio_descriptor_write_at_op);

  lib_aio_descriptor_write_at_op(const asio::error_code& success_ec,
      int descriptor, descriptor_ops::state_type state, int event_fd, io_context_t* aio_ctx,
      Handler& handler, const IoExecutor& io_ex)
    : lib_aio_descriptor_write_at_op_base<ConstBufferSequence>(
        success_ec, descriptor, state, event_fd, aio_ctx,
        &lib_aio_descriptor_write_at_op::do_complete),
      handler_(ASIO_MOVE_CAST(Handler)(handler)),
      work_(handler_, io_ex)
  {
  }

  static void do_complete(void* owner, operation* base,
      const asio::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the handler object.
    lib_aio_descriptor_write_at_op* o
      (static_cast<lib_aio_descriptor_write_at_op*>(base));
    ptr p = { asio::detail::addressof(o->handler_), o, o };

    ASIO_HANDLER_COMPLETION((*o));

    // Take ownership of the operation's outstanding work.
    handler_work<Handler, IoExecutor> w(
        ASIO_MOVE_CAST2(handler_work<Handler, IoExecutor>)(
          o->work_));

    ASIO_ERROR_LOCATION(o->ec_);

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder2<Handler, asio::error_code, std::size_t>
      handler(o->handler_, o->ec_, o->bytes_transferred_);
    p.h = asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
      w.complete(handler, handler.handler_);
      ASIO_HANDLER_INVOCATION_END;
    }
  }

  static void do_immediate(operation* base, bool, const void* io_ex)
  {
    // Take ownership of the handler object.
    lib_aio_descriptor_write_at_op* o(static_cast<lib_aio_descriptor_write_at_op*>(base));
    ptr p = { asio::detail::addressof(o->handler_), o, o };

    ASIO_HANDLER_COMPLETION((*o));

    // Take ownership of the operation's outstanding work.
    immediate_handler_work<Handler, IoExecutor> w(
        ASIO_MOVE_CAST2(handler_work<Handler, IoExecutor>)(
          o->work_));

    ASIO_ERROR_LOCATION(o->ec_);

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder2<Handler, asio::error_code, std::size_t>
      handler(o->handler_, o->ec_, o->bytes_transferred_);
    p.h = asio::detail::addressof(handler.handler_);
    p.reset();

    ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
    w.complete(handler, handler.handler_, io_ex);
    ASIO_HANDLER_INVOCATION_END;
  }

private:
  Handler handler_;
  handler_work<Handler, IoExecutor> work_;
};

} // namespace detail
  
} // namespace asio


#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_LIB_AIO)

#endif // ASIO_DETAIL_LIB_AIO_DESCRIPTOR_READ_AT_OP_HPP
