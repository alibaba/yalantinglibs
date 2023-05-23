//
// detail/impl/lib_aio_descriptor_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// added by ysbarney

#ifndef ASIO_DETAIL_IMPL_LIB_AIO_DESCRIPTOR_SERVICE_IPP
#define ASIO_DETAIL_IMPL_LIB_AIO_DESCRIPTOR_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_LIB_AIO)
#include "asio/error.hpp"
#include "asio/detail/lib_aio_descriptor_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

void lib_aio_descriptor_service::shutdown()
{
}

void lib_aio_descriptor_service::construct(
    lib_aio_descriptor_service::implementation_type& impl)
{
  impl.descriptor_ = -1;
  impl.state_ = 0;
}

void lib_aio_descriptor_service::move_construct(
    lib_aio_descriptor_service::implementation_type& impl,
    lib_aio_descriptor_service::implementation_type& other_impl)
  ASIO_NOEXCEPT
{
  impl.descriptor_ = other_impl.descriptor_;
  other_impl.descriptor_ = -1;

  impl.state_ = other_impl.state_;
  other_impl.state_ = 0;
}

void lib_aio_descriptor_service::move_assign(implementation_type& impl,
      lib_aio_descriptor_service& /*other_service*/,
      implementation_type& other_impl)
{
  destroy(impl);

  impl.descriptor_ = other_impl.descriptor_;
  other_impl.descriptor_ = -1;

  impl.state_ = other_impl.state_;
  other_impl.state_ = 0;
}

void lib_aio_descriptor_service::destroy(
    lib_aio_descriptor_service::implementation_type& impl)
{
  if(is_open(impl))
  {
    ASIO_HANDLER_OPERATION((lib_aio_service_.context(),
          "descriptor", &impl, impl.descriptor_, "destroy"));

    asio::error_code ignored_ec;
    descriptor_ops::close(impl.descriptor_, impl.state_, ignored_ec);
    destroy_aio_context();
  }
}

asio::error_code lib_aio_descriptor_service::assign(
    lib_aio_descriptor_service::implementation_type& impl,
    const native_handle_type& native_descriptor, asio::error_code& ec)
{
  if(is_open(impl))
  {
    ec = asio::error::already_open;
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  impl.descriptor_ = native_descriptor;
  impl.state_ = descriptor_ops::possible_dup;
  ec = success_ec_;
  return ec;
}

asio::error_code lib_aio_descriptor_service::close(
    lib_aio_descriptor_service::implementation_type& impl,
    asio::error_code& ec)
{
  if(is_open(impl))
  {
    ASIO_HANDLER_OPERATION((lib_aio_service_.context(),
          "descriptor", &impl, impl.descriptor_, "close"));

    descriptor_ops::close(impl.descriptor_, impl.state_, ec);
  } 
  else
  {
    ec = success_ec_;
  }

  construct(impl);

  ASIO_ERROR_LOCATION(ec);
  return ec;
}

lib_aio_descriptor_service::native_handle_type 
lib_aio_descriptor_service::release(
    lib_aio_descriptor_service::implementation_type& impl)
{
  native_handle_type descriptor = impl.descriptor_;

  if(is_open(impl))
  {
    ASIO_HANDLER_OPERATION((lib_aio_service_.context(),
          "descriptor", &impl, impl.descriptor_, "release"));
    
    construct(impl);
  }
  return descriptor;
}

asio::error_code lib_aio_descriptor_service::cancel(
    lib_aio_descriptor_service::implementation_type& impl,
    asio::error_code& ec)
{
  if(!is_open(impl))
  {
    ec = asio::error::bad_descriptor;
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  ASIO_HANDLER_OPERATION((lib_aio_service_.context(),
          "descriptor", &impl, impl.descriptor_, "cancel"));
  
  ec = success_ec_;
  return ec;
}

void lib_aio_descriptor_service::do_start_op(
    lib_aio_descriptor_service::implementation_type& impl, int op_type,
    reactor_op* op, bool is_continuation, bool is_non_blocking, bool noop,
    void (*on_immediate)(operation* op, bool, const void*),
    const void* immediate_arg)
{
  if(!noop)
  {
    reactor_.start_op(op_type, event_fd_, reactor_data_,
        op, is_continuation, is_non_blocking, on_immediate, immediate_arg);
  }
  else
  {
    on_immediate(op, is_continuation, immediate_arg);
  }
}

} // namespace detail
} // namespace asio


#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_LIB_AIO)

#endif // ASIO_DETAIL_IMPL_LIB_AIO_DESCRIPTOR_SERVICE_IPP