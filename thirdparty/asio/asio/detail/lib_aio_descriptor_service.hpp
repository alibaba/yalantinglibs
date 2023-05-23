//
// detail/lib_aio_descriptor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// added by ysbarney

#ifndef ASIO_DETAIL_LIB_AIO_DESCRIPTOR_SERVICE_HPP
#define ASIO_DETAIL_LIB_AIO_DESCRIPTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_LIB_AIO)
#include "asio/associated_cancellation_slot.hpp"
#include "asio/buffer.hpp"
#include "asio/cancellation_type.hpp"
#include "asio/execution_context.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/descriptor_ops.hpp"
#include "asio/detail/lib_aio_descriptor_read_at_op.hpp"
#include "asio/detail/lib_aio_descriptor_read_op.hpp"
#include "asio/detail/lib_aio_descriptor_write_at_op.hpp"
#include "asio/detail/lib_aio_descriptor_write_op.hpp"
#include "asio/detail/lib_aio_io_operation.hpp"
#include "asio/detail/lib_aio_null_buffers_op.hpp"
#include "asio/detail/memory.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/posix/descriptor_base.hpp"
#include "asio/detail/reactive_wait_op.hpp"
#include "asio/detail/reactor.hpp"
#include "asio/file_base.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail{

class lib_aio_descriptor_service :
  public execution_context_service_base<lib_aio_descriptor_service>,
  public lib_aio_io_operation
{
public:
  // The native type of a descriptor.
  typedef int native_handle_type;

  // The implementation type of the descriptor.
  class implementation_type
    : private asio::detail::noncopyable
  {
  public:
    // Default constructor.
    implementation_type()
      : descriptor_(-1),
        state_(0),
        descriptor_offset_(0)
    {
    }

  private:
    // Only this service will have access to the internal values.
    friend class lib_aio_descriptor_service;

    // The native descriptor representation.
    int descriptor_;

    // The current state of the descriptor.
    descriptor_ops::state_type state_;

    // write/read offset
    uint64_t descriptor_offset_;
  };

  // Constructor.
  ASIO_DECL lib_aio_descriptor_service(execution_context& context)
    : execution_context_service_base<lib_aio_descriptor_service>(context),
      lib_aio_io_operation(context)
  { 
  }

  // Destroy all user-defined handler objects owned by the service.
  ASIO_DECL void shutdown();

  // Construct a new descriptor implementation.
  ASIO_DECL void construct(implementation_type& impl);

  // Move-construct a new descriptor implementation.
  ASIO_DECL void move_construct(implementation_type& impl,
      implementation_type& other_impl) ASIO_NOEXCEPT;
  
  // Move-assign from another descriptor implementation.
  ASIO_DECL void move_assign(implementation_type& impl,
      lib_aio_descriptor_service& other_service,
      implementation_type& other_impl);
  
  // Destroy a descriptor implementation.
  ASIO_DECL void destroy(implementation_type& impl);

  // Assign a native descriptor to a descriptor implementation.
  ASIO_DECL asio::error_code assign(implementation_type& impl,
      const native_handle_type& native_descriptor,
      asio::error_code& ec);

  // Determine whether the descriptor is open.
  bool is_open(const implementation_type& impl) const
  {
    return impl.descriptor_ != -1;
  }

  // Destroy a descriptor implementation.
  ASIO_DECL asio::error_code close(implementation_type& impl,
      asio::error_code& ec);

  // Get the native descriptor representation.
  native_handle_type native_handle(const implementation_type& impl) const
  {
    return impl.descriptor_;
  }

  // Release ownership of the native descriptor representation.
  ASIO_DECL native_handle_type release(implementation_type& impl);

  // Cancel all operations associated with the descriptor.
  ASIO_DECL asio::error_code cancel(implementation_type& impl,
      asio::error_code& ec);

  // Perform an IO control command on the descriptor.
  template<typename IO_Control_Command>
  asio::error_code io_control(implementation_type& impl,
      IO_Control_Command& command, asio::error_code& ec)
  {
    descriptor_ops::ioctl(impl.descriptor_, impl.state_,
        command.name(), static_cast<ioctl_arg_type*>(command.data()), ec);
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Gets the non-blocking mode of the descriptor.
  bool non_blocking(const implementation_type& impl) const
  {
    return (impl.state_ & descriptor_ops::user_set_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the descriptor.
  asio::error_code non_blocking(implementation_type& impl,
      bool mode, asio::error_code& ec)
  {
    descriptor_ops::set_user_non_blocking(
        impl.descriptor_, impl.state_, mode, ec);
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Gets the non-blocking mode of the native descriptor implementation.
  bool native_non_blocking(const implementation_type& impl) const
  {
    return (impl.state_ & descriptor_ops::internal_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the native descriptor implementation.
  asio::error_code native_non_blocking(implementation_type& impl,
      bool mode, asio::error_code& ec)
  {
    descriptor_ops::set_internal_non_blocking(
        impl.descriptor_, impl.state_, mode, ec);
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Get the size of the file.
  uint64_t size(const implementation_type& impl,
      asio::error_code& ec) const 
  {
    struct stat s;
    int result = ::fstat(native_handle(impl), &s);
    descriptor_ops::get_last_error(ec, result != 0);
    ASIO_ERROR_LOCATION(ec);
    return !ec ? s.st_size : 0;
  }

  // Seek to a position in the file.
  uint64_t seek(implementation_type& impl, int64_t offset,
      file_base::seek_basis whence, asio::error_code& ec)
  {
    if(whence == SEEK_SET)
    {
      impl.descriptor_offset_ = offset;
    }
    else if(whence == SEEK_CUR)
    {
      impl.descriptor_offset_ += offset;
    }
    else if(whence == SEEK_END)
    {
      uint64_t result = size(impl, ec);
      if(ec)
        return 0;
      
      impl.descriptor_offset_ = result + offset;
    } 
    return impl.descriptor_offset_;
  }

  // Wait for the descriptor to become ready to read, ready to write, or to have
  // pending error conditions.
  asio::error_code wait(implementation_type& impl,
      posix::descriptor_base::wait_type w, asio::error_code& ec)
  {
    switch(w)
    {
    case posix::descriptor_base::wait_read:
      descriptor_ops::poll_read(impl.descriptor_, impl.state_, ec);
      break;
    case posix::descriptor_base::wait_write:
      descriptor_ops::poll_write(impl.descriptor_, impl.state_, ec);
      break;
    case posix::descriptor_base::wait_error:
      descriptor_ops::poll_error(impl.descriptor_, impl.state_, ec);
      break;
    default:
      ec = asio::error::invalid_argument;
      break;
    }
    
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Asynchronously wait for the descriptor to become ready to read, ready to
  // write, or to have pending error conditions.
  template<typename Handler, typename IoExecutor>
  void async_wait(implementation_type& impl,
      posix::descriptor_base::wait_type w,
      Handler& handler, const IoExecutor& io_ex)
  {
    bool is_continuation = 
      asio_handler_cont_helpers::is_continuation(handler);

    typename associated_cancellation_slot<Handler>::type slot
      = asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef reactive_wait_op<Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, handler, io_ex);

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p,
          "descriptor", &impl, impl.descriptor_, "async_wait"));

    int op_type;
    switch(w)
    {
    case posix::descriptor_base::wait_read:
      op_type = reactor::read_op;
      break;
    case posix::descriptor_base::wait_write:
      op_type = reactor::write_op;
      break;
    case posix::descriptor_base::wait_error:
      op_type = reactor::except_op;
      break;
    default:
      p.p->ec_ = asio::error::invalid_argument;
      start_op(impl, reactor::read_op, p.p, is_continuation, false, true, &io_ex, 0);
      p.v = p.p = 0;
      return;
    }

    // Optionally register for per-operation cancellation.
    if(slot.is_connected())
    {
      p.p->cancellation_key =
        &slot.template emplace<lib_aio_op_cancellation>(
            &reactor_, &reactor_data_, event_fd_, op_type);
    }

    start_op(impl, op_type, p.p, is_continuation, false);
    p.v = p.p = 0;
  }

  // Write some data to the descriptor.
  template<typename ConstBufferSequence>
  size_t write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence> bufs_type;

    size_t n;
    if(bufs_type::is_single_buffer)
    {
      n = descriptor_ops::sync_write1(impl.descriptor_,
          impl.state_, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), ec);
    }
    else
    {
      bufs_type bufs(buffers);

      n = descriptor_ops::sync_write(impl.descriptor_, impl.state_,
          bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
    }

    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Wait until data can be written without blocking.
  size_t write_some(implementation_type& impl,
      const null_buffers&, asio::error_code& ec)
  {
    // Wait for descriptor to become ready.
    descriptor_ops::poll_write(impl.descriptor_, impl.state_, ec);

    ASIO_ERROR_LOCATION(ec);
    return 0;
  }

  // Start an asynchronous write. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template<typename ConstBufferSequence, typename Handler, typename IoExecutor>
  void async_write_some(implementation_type& impl, 
      const ConstBufferSequence& buffers, Handler& handler,
      const IoExecutor& io_ex)
  {
    bool is_continuation = 
      asio_handler_cont_helpers::is_continuation(handler);

    typename associated_cancellation_slot<Handler>::type slot
      = asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef lib_aio_descriptor_write_op<
      ConstBufferSequence, Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, impl.descriptor_,
        impl.state_, event_fd_, &aio_ctx_, handler, io_ex);

    // Optionally register for per-operation cancellation.
    if(slot.is_connected())
    {
      p.p->cancellation_key_ = 
        &slot.template emplace<lib_aio_op_cancellation>(&reactor_,
            &reactor_data_, event_fd_, reactor::write_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p,
          "descriptor", &impl, impl.descriptor_, "async_write_some"));

    asio::error_code ec;
    if(!aio_io_submit_write(impl.descriptor_, buffers, impl.descriptor_offset_, ec)) 
    {
      reactor_.post_immediate_completion(p.p, is_continuation);
      p.v = p.p = 0;
      return;
    }

    start_op(impl, reactor::write_op, p.p, is_continuation, true,
        buffer_sequence_adapter<asio::const_buffer,
          ConstBufferSequence>::all_empty(buffers), &io_ex, 0);
    p.p = p.v = 0;
  }

  // Start an asynchronous wait until data can be written without blocking.
  template<typename Handler, typename IoExecutor>
  void async_write_some(implementation_type& impl, 
      const null_buffers&, Handler& handler, const IoExecutor& io_ex)
  {
    bool is_continuation = 
      asio_handler_cont_helpers::is_continuation(handler);

    typename associated_cancellation_slot<Handler>::type slot
      = asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef lib_aio_null_buffers_op<Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, handler, io_ex);

    // Optionally register for per-operation cancellation.
    if(slot.is_connected())
    {
      p.p->cancellation_key_ = 
        &slot.template emplace<lib_aio_op_cancellation>(&reactor_,
            &reactor_data_, event_fd_, reactor::write_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p,
          "descriptor", &impl, impl.descriptor_, "async_write_some(null_buffers)"));

    start_op(impl, reactor::write_op, p.p, is_continuation, true, false, &io_ex, 0);
    p.p = p.v = 0;
  }

  // Write some data to the descriptor at the specified offset.
  template<typename ConstBufferSequence>
  size_t write_some_at(implementation_type& impl, uint64_t offset,
      const ConstBufferSequence& buffers, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence> bufs_type;
    
    size_t n;
    if(bufs_type::is_single_buffer)
    {
      n = descriptor_ops::sync_write_at1(impl.descriptor_, 
          impl.state_, offset, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), ec);
    }
    else
    {
      bufs_type bufs(buffers);

      n = descriptor_ops::sync_write_at(impl.descriptor_, impl.state_,
          offset, bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
    }

    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Wait unit data can be written without blocking.
  size_t write_some_at(implementation_type& impl, uint64_t,
      const null_buffers& buffer, asio::error_code& ec)
  {
    return write_some(impl, buffer, ec);
  }

  // Start an asynchronous write at the specified offset. The data being sent
  // must be valid for the lifetime of the asynchronous operation.
  template<typename ConstBufferSequence, typename Handler, typename IoExecutor>
  void async_write_some_at(implementation_type& impl, uint64_t offset,
      const ConstBufferSequence& buffers, Handler& handler,
      const IoExecutor& io_ex)
  {
    bool is_continuation = 
      asio_handler_cont_helpers::is_continuation(handler);

    typename associated_cancellation_slot<Handler>::type slot
      = asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef lib_aio_descriptor_write_at_op<
      ConstBufferSequence, Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, impl.descriptor_,
        impl.state_, event_fd_, &aio_ctx_, handler, io_ex);

    // Optionally register for per-operation cancellation.
    if(slot.is_connected())
    {
      p.p->cancellation_key_ = 
        &slot.template emplace<lib_aio_op_cancellation>(&reactor_,
            &reactor_data_, event_fd_, reactor::write_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p,
          "descriptor", &impl, impl.descriptor_, "async_write_some_at"));

    asio::error_code ec;
    if(!aio_io_submit_write(impl.descriptor_, buffers, offset, ec))
    {
      reactor_.post_immediate_completion(p.p, is_continuation);
      p.v = p.p = 0;
      return;
    }

    start_op(impl, reactor::write_op, p.p, is_continuation, true,
        buffer_sequence_adapter<asio::const_buffer,
          ConstBufferSequence>::all_empty(buffers), &io_ex, 0);
    p.p = p.v = 0;
  }

  // Start an asynchronous wait until data can be written without blocking.
  template<typename Handler, typename IoExecutor>
  void async_write_some_at(implementation_type& impl,
      const null_buffers& buffers, Handler& handler, const IoExecutor& io_ex)
  {
    return async_write_some(impl, buffers, handler, io_ex);
  }

  // Read some data from the stream. Returns the number of bytes read.
  template<typename MutableBufferSequence>
  size_t read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence> bufs_type;
    
    size_t n;
    if(bufs_type::is_single_buffer)
    {
      n = descriptor_ops::sync_read1(impl.descriptor_, 
          impl.state_, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), ec);
    }
    else
    {
      bufs_type bufs(buffers);

      n = descriptor_ops::sync_read(impl.descriptor_, impl.state_, 
          bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
    }

    ASIO_ERROR_LOCATION(ec);
    return n;
  }

  // Wait until data can be read without blocking.
  size_t read_some(implementation_type& impl,
      const null_buffers&, asio::error_code& ec)
  {
    // Wait for descriptor to become ready.
    descriptor_ops::poll_read(impl.descriptor_, impl.state_, ec);

    ASIO_ERROR_LOCATION(ec);
    return 0;
  }

  // Start an asynchronous read. The buffer for the data being read must be
  // valid for the lifetime of the asynchronous operation.
  template<typename MutableBufferSequence,
    typename Handler, typename IoExecutor>
  void async_read_some(implementation_type& impl,
      const MutableBufferSequence& buffers,
      Handler& handler, const IoExecutor& io_ex)
  {
    bool is_continuation = 
      asio_handler_cont_helpers::is_continuation(handler);

    typename associated_cancellation_slot<Handler>::type slot
      = asio::get_associated_cancellation_slot(handler);

    // Allocate andd construct an operation to wrap the handler.
    typedef lib_aio_descriptor_read_op<
      MutableBufferSequence, Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, impl.descriptor_,
        impl.state_, event_fd_, &aio_ctx_, handler, io_ex);
    
    // Optionally register for per-operation cancellation.
    if(slot.is_connected())
    {
      p.p->cancellation_key_ = 
        &slot.template emplace<lib_aio_op_cancellation>(&reactor_,
            &reactor_data_, event_fd_, reactor::read_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p,
          "descriptor", &impl, impl.descriptor_, "async_read_some"));

    asio::error_code ec;
    if(!aio_io_submit_read(impl.descriptor_, buffers, impl.descriptor_offset_, ec))
    {
      reactor_.post_immediate_completion(p.p, is_continuation);
      p.v = p.p = 0;
      return;
    }

    start_op(impl, reactor::read_op, p.p, is_continuation, true,
        buffer_sequence_adapter<asio::mutable_buffer,
          MutableBufferSequence>::all_empty(buffers), &io_ex, 0);
    p.p = p.v = 0;
  }

  // Wait until data can be read without blocking.
  template <typename Handler, typename IoExecutor>
  void async_read_some(implementation_type& impl,
      const null_buffers&, Handler& handler, const IoExecutor& io_ex)
  {
    bool is_continuation =
      asio_handler_cont_helpers::is_continuation(handler);

    typename associated_cancellation_slot<Handler>::type slot
      = asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef lib_aio_null_buffers_op<Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, handler, io_ex);

    // Optionally register for per-operation cancellation.
    if (slot.is_connected())
    {
      p.p->cancellation_key_ =
        &slot.template emplace<lib_aio_op_cancellation>(&reactor_,
            &reactor_data_, event_fd_, reactor::read_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(),
          *p.p, "descriptor", &impl, impl.descriptor_,
          "async_read_some(null_buffers)"));

    start_op(impl, reactor::read_op, p.p, is_continuation, false, false, &io_ex, 0);
    p.v = p.p = 0;
  }

  // Read some data at the specified offset. Returns the number of bytes read.
  template <typename MutableBufferSequence>
  size_t read_some_at(implementation_type& impl, uint64_t offset,
      const MutableBufferSequence& buffers, asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence> bufs_type;

    if (bufs_type::is_single_buffer)
    {
      return descriptor_ops::sync_read_at1(impl.descriptor_,
          impl.state_, offset, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), ec);
    }
    else
    {
      bufs_type bufs(buffers);

      return descriptor_ops::sync_read_at(impl.descriptor_, impl.state_,
          offset, bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
    }
  }

  // Wait until data can be read without blocking.
  size_t read_some_at(implementation_type& impl, uint64_t,
      const null_buffers& buffers, asio::error_code& ec)
  {
    return read_some(impl, buffers, ec);
  }

  // Start an asynchronous read. The buffer for the data being read must be
  // valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence,
      typename Handler, typename IoExecutor>
  void async_read_some_at(implementation_type& impl,
      uint64_t offset, const MutableBufferSequence& buffers,
      Handler& handler, const IoExecutor& io_ex)
  {
    bool is_continuation =
      asio_handler_cont_helpers::is_continuation(handler);

    typename associated_cancellation_slot<Handler>::type slot
      = asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef lib_aio_descriptor_read_at_op<
      MutableBufferSequence, Handler, IoExecutor> op;
    typename op::ptr p = { asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(success_ec_, impl.descriptor_,
        impl.state_, event_fd_, &aio_ctx_, handler, io_ex);

    // Optionally register for per-operation cancellation.
    if (slot.is_connected())
    {
      p.p->cancellation_key_ =
        &slot.template emplace<lib_aio_op_cancellation>(&reactor_,
            &reactor_data_, event_fd_, reactor::read_op);
    }

    ASIO_HANDLER_CREATION((reactor_.context(), *p.p,
          "descriptor", &impl, impl.descriptor_, "async_read_some_at"));

    asio::error_code ec;
    if(!aio_io_submit_read(impl.descriptor_, buffers, offset, ec))
    {
      reactor_.post_immediate_completion(p.p, is_continuation);
      p.v = p.p = 0;
      return;
    }

    start_op(impl, reactor::read_op, p.p, is_continuation, true,
        buffer_sequence_adapter<asio::mutable_buffer,
          MutableBufferSequence>::all_empty(buffers), &io_ex, 0);
    p.v = p.p = 0;
  }

  // Wait until data can be read without blocking.
  template <typename Handler, typename IoExecutor>
  void async_read_some_at(implementation_type& impl, uint64_t,
      const null_buffers& buffers, Handler& handler, const IoExecutor& io_ex)
  {
    return async_read_some(impl, buffers, handler, io_ex);
  }

private:
  // Start the asynchronous read or write operation.
  ASIO_DECL void do_start_op(implementation_type& impl, int op_type,
      reactor_op* op, bool is_continuation, bool is_non_blocking, bool noop,
      void (*on_immediate)(operation* op, bool, const void*),
      const void* immediate_arg);

  // Start the asynchronous operation for handlers that are specialised for
  // immediate completion.
  template<typename Op>
  ASIO_DECL void start_op(implementation_type& impl, int op_type,
      Op* op, bool is_continuation, bool is_non_blocking, bool noop,
      const void* io_ex, ...)
  {
    return do_start_op(impl, op_type, op, is_continuation, is_non_blocking,
        noop, &Op::do_immediate, io_ex);
  }

  // Helper class used to implement per-operation cancellation
  class lib_aio_op_cancellation
  {
  public:
    lib_aio_op_cancellation(reactor* r,
        reactor::per_descriptor_data* p, int d, int o)
      : reactor_(r),
        reactor_data_(p),
        descriptor_(d),
        op_type_(o)
    {
    }

    void operator()(cancellation_type_t type)
    {
      if (!!(type &
            (cancellation_type::terminal
              | cancellation_type::partial
              | cancellation_type::total)))
      {
        reactor_->cancel_ops_by_key(descriptor_,
            *reactor_data_, op_type_, this);
      }
    }

  private:
    reactor* reactor_;
    reactor::per_descriptor_data* reactor_data_;
    int descriptor_;
    int op_type_;
  };

  // Cached success value to avoid accessing category singleton.
  const asio::error_code success_ec_;
};

} // namespace detail
} // namespace asio


#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/lib_aio_descriptor_service.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // defined(ASIO_HAS_LIB_AIO)

#endif // ASIO_DETAIL_LIB_AIO_DESCRIPTOR_SERVICE_HPP
