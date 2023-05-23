//
// detail/impl/lib_aio_file_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2023 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// added by ysbarney

#ifndef ASIO_DETAIL_IMPL_LIB_AIO_FILE_SERVICE_IPP
#define ASIO_DETAIL_IMPL_LIB_AIO_FILE_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_FILE) \
  && defined(ASIO_HAS_LIB_AIO)

#include <cstring>
#include <sys/stat.h>
#include "asio/detail/lib_aio_file_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

lib_aio_file_service::lib_aio_file_service(
    execution_context& context)
  : execution_context_service_base<lib_aio_file_service>(context),
    descriptor_service_(context)
{
}

void lib_aio_file_service::shutdown()
{
  descriptor_service_.shutdown();
}

asio::error_code lib_aio_file_service::open(
    lib_aio_file_service::implementation_type& impl,
    const char* path, file_base::flags open_flags,
    asio::error_code& ec)
{
  if(is_open(impl))
  {
    ec = asio::error::already_open;
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  descriptor_ops::state_type state = 0;
  int fd = descriptor_ops::open(path, open_flags, 0777, ec);
  if(fd < 0)
  {
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // We're done. Take ownership of the serial port descriptor.
  if(descriptor_service_.assign(impl, fd, ec))
  {
    asio::error_code ignored_ec;
    descriptor_ops::close(fd, state, ignored_ec);
  }

  ASIO_ERROR_LOCATION(ec);
  return ec;
}

/*uint64_t lib_aio_file_service::size(
    const lib_aio_file_service::implementation_type &impl,
    asio::error_code& ec) const
{
  struct stat s;
  int result = ::fstat(native_handle(impl), &s);
  descriptor_ops::get_last_error(ec, result != 0);
  ASIO_ERROR_LOCATION(ec);
  return !ec ? s.st_size : 0;
}*/

asio::error_code lib_aio_file_service::resize(
    lib_aio_file_service::implementation_type& impl,
    uint64_t n, asio::error_code& ec)
{
  int result = ::ftruncate(native_handle(impl), n);
  descriptor_ops::get_last_error(ec, result != 0);
  ASIO_ERROR_LOCATION(ec);
  return ec;
}

asio::error_code lib_aio_file_service::sync_all(
    lib_aio_file_service::implementation_type& impl,
    asio::error_code& ec)
{
  int result = ::fsync(native_handle(impl));
  descriptor_ops::get_last_error(ec, result != 0);
  return ec;
}
  
asio::error_code lib_aio_file_service::sync_data(
    lib_aio_file_service::implementation_type& impl,
    asio::error_code& ec)
{
#if defined(_POSIX_SYNCHRONIZED_IO)
  int result = ::fdatasync(native_handle(impl));
#else // defined(_POSIX_SYNCHRONIZED_IO)
  int result = ::fsync(native_handle(impl));
#endif // defined(_POSIX_SYNCHRONIZED_IO)
  descriptor_ops::get_last_error(ec, result != 0);
  ASIO_ERROR_LOCATION(ec);
  return ec;
}
  
/*uint64_t lib_aio_file_service::seek(
    lib_aio_file_service::implementation_type& impl, 
    int64_t offset, file_base::seek_basis whence, asio::error_code& ec)
{
  int64_t result = ::lseek(native_handle(impl), offset, whence);
  descriptor_ops::get_last_error(ec, result < 0);
  ASIO_ERROR_LOCATION(ec);
  return !ec ? static_cast<uint64_t>(result) : 0;
}*/

} // namespace detail
} // namespace asio


#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_FILE)
       //   && defined(ASIO_HAS_LIB_AIO)

#endif // ASIO_DETAIL_IMPL_LIB_AIO_FILE_SERVICE_IPP
