#include <string>
#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/cuda/cuda_memory.hpp"
#include "ylt/coro_io/data_view.hpp"
#include "ylt/coro_io/heterogeneous_buffer.hpp"
#include "ylt/coro_io/ibverbs/ib_socket.hpp"
#include "ylt/coro_rpc/coro_rpc_client.hpp"
#include "ylt/coro_rpc/coro_rpc_server.hpp"
#include "doctest.h"
#include "ylt/coro_rpc/impl/coro_rpc_client.hpp"
#include "ylt/coro_rpc/impl/default_config/coro_rpc_config.hpp"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"

constexpr int data_size = 1024*1024;
std::string gdr_echo(std::string meta_info) {
  CHECK(meta_info=="gdr_echo");
  coro_io::data_view data = coro_rpc::get_context()->get_request_attachment2();
  std::string str(data_size,'b');
  CHECK(data.size()==data_size);
  coro_io::cuda_copy(str.data(), -1, data.data(), data.gpu_id(), data.size());
  CHECK(str==std::string(data_size,'a'));
  coro_rpc::get_context()->set_response_attachment(data);
  return "gdr_echo";
}

auto gdr_dev = coro_io::ib_device_t::create(
  {.buffer_pool_config = {.buffer_size = 256*1024, .gpu_id = 0}});
TEST_CASE("test gdr") {
  coro_rpc::coro_rpc_server s(1,9001);
  s.init_ibv(coro_io::ib_socket_t::config_t{.device=gdr_dev});
  s.register_handler<gdr_echo>();
  s.async_start();
  coro_rpc::coro_rpc_client cli;
  auto result =cli.init_ibv(coro_io::ib_socket_t::config_t{.device=gdr_dev});
  async_simple::coro::syncAwait(cli.connect("127.0.0.1", "9001"));
  coro_io::heterogeneous_buffer buf(data_size,0),buf2(data_size,0);
  coro_io::data_view x{buf};
  std::string str (data_size,'a'),str2(data_size,'b');
  coro_io::cuda_copy(buf.data(), buf.gpu_id(), str.data(), -1, str.size());
  cli.set_req_attachment(coro_io::data_view{buf});
  auto ret = async_simple::coro::syncAwait(cli.call<gdr_echo>("gdr_echo"));
  CHECK(ret == "gdr_echo");
  auto buf3 = cli.get_resp_attachment2();
  CHECK(buf3.gpu_id() == coro_io::data_view{buf2}.gpu_id());
  coro_io::cuda_copy(str2.data(), -1, buf3.data(), buf3.gpu_id(), buf3.size());
  CHECK(str2 == str);
}
TEST_CASE("test gdr with user attachment") {
  coro_rpc::coro_rpc_server s(1,9001);
  s.init_ibv(coro_io::ib_socket_t::config_t{.device=gdr_dev});
  s.register_handler<gdr_echo>();
  s.async_start();
  coro_rpc::coro_rpc_client cli;
  auto result =cli.init_ibv(coro_io::ib_socket_t::config_t{.device=gdr_dev});
  async_simple::coro::syncAwait(cli.connect("127.0.0.1", "9001"));
  coro_io::heterogeneous_buffer buf(data_size,0),buf2(data_size,0);
  coro_io::data_view x{buf};
  std::string str (data_size,'a'),str2(data_size,'b');
  coro_io::cuda_copy(buf.data(), buf.gpu_id(), str.data(), -1, str.size());
  cli.set_req_attachment(coro_io::data_view{buf});
  cli.set_resp_attachment_buf2(coro_io::data_view{buf2});
  auto ret = async_simple::coro::syncAwait(cli.call<gdr_echo>("gdr_echo"));
  CHECK(ret == "gdr_echo");
  auto buf3 = cli.get_resp_attachment2();
  CHECK(buf3.data() == coro_io::data_view{buf2}.data());
  CHECK(buf3.size() == coro_io::data_view{buf2}.size());
  CHECK(buf3.gpu_id() == coro_io::data_view{buf2}.gpu_id());
  coro_io::cuda_copy(str2.data(), -1, buf3.data(), buf3.gpu_id(), buf3.size());
  CHECK(str2 == str);
}
std::string rand_str(std::size_t length) {
  const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::string result;
  result.reserve(length);
  for (std::size_t i = 0; i < length; ++i) {
    result += charset[std::rand() % charset.length()];
  }
  return result;
}
std::string_view echo(std::string_view name) {
  auto view = coro_rpc::get_context()->get_request_attachment2();
  coro_rpc::get_context()->set_response_attachment2(view);
  return name;
};
coro_io::heterogeneous_buffer make_gpu_buffer(std::string_view src) {
  coro_io::heterogeneous_buffer buf(src.size(), 0);
  coro_io::cuda_copy(buf.data(), 0, src.data(), -1, src.size());
  return buf;
}
std::string make_cpu_buffer(coro_io::data_view src) {
  std::string buf(src.size(),'\0');
  coro_io::cuda_copy(buf.data(), -1, src.data(), src.gpu_id(), src.size());
  return buf;
}
TEST_CASE("test gdr") {
  auto dev =coro_io::ib_device_t::create({.buffer_pool_config ={.gpu_id=0}});
  coro_rpc::coro_rpc_server server(1,9001);
  server.init_ibv({.device = dev});
  server.register_handler<echo>();
  server.async_start();
  coro_rpc::coro_rpc_client cli;
  [[maybe_unused]] bool _ = cli.init_ibv({.device = dev});
  auto arg = rand_str(1024*1024*16), attach = rand_str(1024*1024*16);
  async_simple::coro::syncAwait(cli.connect("127.0.0.1", "9001"));
  SUBCASE("test normal rpc") {
    cli.set_req_attachment("hello2");
    auto result = async_simple::coro::syncAwait(cli.call<echo>("hello"));
    CHECK(result.value() == "hello");
    CHECK(make_cpu_buffer(cli.get_resp_attachment2())=="hello2");
  }
  SUBCASE("test normal rpc with 16M data") {
    cli.set_req_attachment(attach);
    auto result = async_simple::coro::syncAwait(cli.call<echo>(arg));
    CHECK(result.value() == arg);
    CHECK(make_cpu_buffer(cli.get_resp_attachment2())==attach);
  }
  SUBCASE("test client & server attachment with gdr") {
    auto buffer = make_gpu_buffer("hello2");
    cli.set_req_attachment2(buffer);
    auto result = async_simple::coro::syncAwait(cli.call<echo>("hello"));
    CHECK(result.value() == "hello");
    CHECK(make_cpu_buffer(cli.get_resp_attachment2())=="hello2");
  }
  SUBCASE("test client & server attachment with gdr 16M data") {
    auto buffer = make_gpu_buffer(attach);
    cli.set_req_attachment2(buffer);
    auto result = async_simple::coro::syncAwait(cli.call<echo>(arg));
    CHECK(result.value() == arg);
    CHECK(make_cpu_buffer(cli.get_resp_attachment2())==attach);
  }
  SUBCASE("test client set attachment buf & server attachment with gdr") {
    auto buffer = make_gpu_buffer(attach);
    cli.set_req_attachment2(buffer);
    coro_io::heterogeneous_buffer buf(1024*1024*16+1, 0);
    cli.set_resp_attachment_buf2(buf);
    auto result = async_simple::coro::syncAwait(cli.call<echo>(arg));
    CHECK(result.value() == arg);
    CHECK(make_cpu_buffer(cli.get_resp_attachment2())==attach);
    CHECK(cli.get_resp_attachment2().data()== buffer.data());
  }
  SUBCASE("test client set attachment buf & server attachment with gdr and buffer is small than expected") {
    auto buffer = make_gpu_buffer(attach);
    cli.set_req_attachment2(buffer);
    coro_io::heterogeneous_buffer buf(1024*1024*16-1, 0);
    cli.set_resp_attachment_buf2(buf);
    auto result = async_simple::coro::syncAwait(cli.call<echo>(arg));
    CHECK(result.value() == arg);
    CHECK(make_cpu_buffer(cli.get_resp_attachment2())==attach);
    CHECK(cli.get_resp_attachment2().data() != buffer.data());
  }
}
