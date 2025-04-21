#include <chrono>
#include <system_error>
#include "asio/ip/address.hpp"
#include "asio/ip/address_v4.hpp"
#include "asio/ip/tcp.hpp"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/ibverbs/ib_buffer.hpp"
#include "ylt/coro_io/ibverbs/ib_io.hpp"
#include "ylt/coro_io/ibverbs/ib_socket.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
#include "ylt/easylog/record.hpp"


char buffer[8*1024*1024];
async_simple::coro::Lazy<std::error_code> echo_connect(coro_io::ib_socket_t soc) {
  
  ELOG_INFO<<"start echo connect";
  coro_io::ib_buffer_t ib=coro_io::ib_buffer_t::regist(soc.get_device().pd(),buffer,sizeof(buffer));
  ELOG_INFO<<"start read from client";
  auto [ec,len] = co_await coro_io::async_read_some(soc,coro_io::ib_buffer_view_t{ib});
  if (ec) [[unlikely]] {
    ELOG_INFO<<ec.message();
    co_return ec;
  }
  ELOG_INFO<<"read from client ok ";
  for (int i = 0;; i ^= 1) {
    auto r_view = coro_io::ib_buffer_view_t{ib};
    auto s_view = std::span<char>{buffer};
    
    ELOG_INFO<<"start read from client";
    auto [r,s] = co_await async_simple::coro::collectAll(coro_io::async_read_some(soc,r_view),coro_io::async_write(soc,s_view));
    ELOG_INFO<<"read data ok";
    
    if (r.hasError() || s.hasError()) [[unlikely]] {
      co_return std::make_error_code(std::errc::io_error);
    }
    if (s.value().first) [[unlikely]] {
      co_return r.value().first;
    }
    if (s.value().first) [[unlikely]] {
      co_return s.value().first;
    }
    ELOG_INFO<<"read from client ok ";
    len = r.value().second;
    ELOG_INFO<<std::string_view{(char*)r_view.address(),len};
  }
  co_return std::error_code{};
}

coro_io::ib_device_t dev{{}};

async_simple::coro::Lazy<std::error_code> echo_accept() {
  
  asio::ip::tcp::acceptor acceptor(coro_io::get_global_executor()->get_asio_executor());
  std::error_code ec;
  auto address=asio::ip::address_v4::from_string("127.0.0.1",ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  auto endpoint =asio::ip::tcp::endpoint(address,58096);
  acceptor.open(endpoint.protocol(),ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  acceptor.bind(endpoint,ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  acceptor.listen(asio::ip::tcp::socket::max_listen_connections,ec);
  if (ec) [[unlikely]] {
    co_return ec;
  }
  
  ELOG_INFO<<"tcp listening";
  while (true) {
    coro_io::ib_socket_t soc(coro_io::get_global_executor(),&dev);
    soc.get_config().buffer_size = 8 * 1024 * 1024;
    auto ec = co_await coro_io::async_accept(acceptor,soc );
    
    if (ec) [[unlikely]] {
      
      ELOG_INFO<<"accept failed";
      co_return ec;
    }
    
    ELOG_INFO<<"start new connection";
    auto executor=soc.get_executor();
    echo_connect(std::move(soc)).start([](auto&&){});
  }
  co_return ec;
}


async_simple::coro::Lazy<std::error_code> echo_client(coro_io::ib_socket_t& soc, std::string_view sv) {
  std::string buffer;
  buffer.resize(sv.size());
  auto ib = coro_io::ib_buffer_t::regist(soc.get_device().pd(),(char*)sv.data(),sv.size());
  auto ib2 = coro_io::ib_buffer_t::regist(soc.get_device().pd(),buffer.data(),buffer.size());
  ELOG_INFO<<"start echo";
  co_await coro_io::sleep_for(std::chrono::seconds{1},soc.get_executor());
  for (int i = 0;; i ^= 1) {
    ELOG_INFO<<"prepare read buffer";
    auto r_view = coro_io::ib_buffer_view_t{ib2};
    ELOG_INFO<<"prepare write buffer";
    auto s_view = std::span<char>{(char*)ib->addr,ib->length};
    ELOG_INFO<<"echoing";
    auto [r,s] = co_await async_simple::coro::collectAll(coro_io::async_read_some(soc,r_view),coro_io::async_write(soc,s_view));
    ELOG_INFO<<"echoing over";
    if (r.hasError() || s.hasError()) [[unlikely]] {
      co_return std::make_error_code(std::errc::io_error);
    }
    if (s.value().first) [[unlikely]] {
      co_return r.value().first;
    }
    if (s.value().first) [[unlikely]] {
      co_return s.value().first;
    }
    if (std::string_view{(char*)r_view.address(),r.value().second}!=sv) [[unlikely]] {
      co_return std::make_error_code(std::errc::protocol_error);
    }
  }
  co_return std::error_code{};
}

async_simple::coro::Lazy<std::error_code> echo_connect() {
  coro_io::ib_socket_t soc(coro_io::get_global_executor(),&dev);
  auto ec = co_await coro_io::async_connect(soc,"127.0.0.1","58096");
  if (ec) [[unlikely]] {

    co_return ec;
  }
  ELOG_INFO<<"connect over";
  std::string s="hello";
  co_return co_await echo_client(soc, s);
}


int main() {
  easylog::logger<>::instance().init(easylog::Severity::TRACE,false,true,"1.log",10000,1,true);
  ELOG_INFO<<"start echo server & client";
  echo_accept().start([](auto&&){});
  async_simple::coro::syncAwait(echo_connect());
  return 0;
}