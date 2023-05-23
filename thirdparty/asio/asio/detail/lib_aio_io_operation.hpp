//
// detail/lib_aio_io_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//added by ysbarney

#ifndef ASIO_DETAIL_LIB_AIO_IO_OPERATION_HPP
#define ASIO_DETAIL_LIB_AIO_IO_OPERATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_LIB_AIO)
#include <libaio.h>
#include <sys/eventfd.h>
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/reactor.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class lib_aio_io_operation
{
public:
  // The number of operations in a batch.
  enum { batch_size = 256 };

  // The lib_aio_service that performs event demultiplexing for the service.
  int event_fd_;

  // libaio context
  io_context_t aio_ctx_;

  // The selector that performs event demultiplexing for the service.
  reactor& reactor_;

  // Per-descriptor data used by the reactor.
  reactor::per_descriptor_data reactor_data_;

  struct iocb aio_iocb_;

  // Construct.
  lib_aio_io_operation(execution_context& context)
    : reactor_(asio::use_service<reactor>(context)),
    aio_iocb_(NULL)
  {
    reactor_.init_task();
    init_aio_context();
  }

  // Initialise the context.
  ASIO_DECL void init_aio_context()
  {
    event_fd_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if(event_fd_ < 0) 
    {
      asio::error_code ec(-1,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "eventfd");
    }

    int ret = ::io_setup(batch_size, &aio_ctx_);
    if (ret < 0)
    {
      asio::error_code ec(ret,
          asio::error::get_system_category());
      ::close(event_fd_);
      asio::detail::throw_error(ec, "io_setup");
    }

    ret = reactor_.register_descriptor(event_fd_, reactor_data_);
    if(ret) {
      asio::error_code ec(ret,
          asio::error::get_system_category());
      ::close(event_fd_);
      ::io_destroy(aio_ctx_);
      asio::detail::throw_error(ec, "register_descriptor");
    }
  }

  ASIO_DECL void destroy_aio_context() {
    ::close(event_fd_);
    ::io_destroy(aio_ctx_);
  }

  //submit async write some.
  template<typename ConstBufferSequence>
  ASIO_DECL bool aio_io_submit_write(int descriptor, 
    const ConstBufferSequence& buffers, uint64_t offset,
    asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::const_buffer,
        ConstBufferSequence> bufs_type;

    if(bufs_type::is_single_buffer)
    {

      ::io_prep_pwrite(&aio_iocb_, descriptor, (void*)bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), offset);
    }
    else
    {
      bufs_type bufs(buffers);
      ::io_prep_pwritev(&aio_iocb_, descriptor, bufs.buffers(), bufs.count(), offset);
    }
    io_set_eventfd(&aio_iocb_, event_fd_);

    struct iocb *io_cb = &aio_iocb_;
    int ret = ::io_submit(aio_ctx_, 1, &io_cb);
    if( ret != 1)
    {
      ec = asio::error_code(errno, asio::error::get_system_category());
      return false;
    }
    return true;
  }
  
  //submit async read some.
  template<typename MutableBufferSequence>
  ASIO_DECL bool aio_io_submit_read(int descriptor, 
    const MutableBufferSequence& buffers, uint64_t offset,
    asio::error_code& ec)
  {
    typedef buffer_sequence_adapter<asio::mutable_buffer,
        MutableBufferSequence> bufs_type;
    
    if(bufs_type::is_single_buffer)
    {
      ::io_prep_pread(&aio_iocb_, descriptor, bufs_type::first(buffers).data(),
          bufs_type::first(buffers).size(), offset);
    }
    else
    {
      bufs_type bufs(buffers);
      ::io_prep_preadv(&aio_iocb_, descriptor, bufs.buffers(), bufs.count(), offset);
    }
    io_set_eventfd(&aio_iocb_, event_fd_);

    struct iocb *io_cb = &aio_iocb_;
    int ret = ::io_submit(aio_ctx_, 1, &io_cb);
    if( ret != 1)
    {
      ec = asio::error_code(errno, asio::error::get_system_category());
      return false;
    }
    return true;
  }
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_LIB_AIO)

#endif // ASIO_DETAIL_LIB_AIO_IO_OPERATION_HPP
