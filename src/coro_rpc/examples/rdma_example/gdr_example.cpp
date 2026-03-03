#include <iostream>
#include <string>

#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/coro_io/cuda/cuda_memory.hpp"
#include "ylt/coro_io/data_view.hpp"
#include "ylt/coro_io/heterogeneous_buffer.hpp"
#include "ylt/coro_io/ibverbs/ib_socket.hpp"
#include "ylt/coro_rpc/coro_rpc_client.hpp"
#include "ylt/coro_rpc/coro_rpc_server.hpp"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"

std::string_view echo(std::string_view arg) {
  // get req attachement
  coro_io::data_view attachment =
      coro_rpc::get_context()->get_request_attachment2();
  std::cout << "Attachement gpu id:" << attachment.gpu_id()
            << "size:" << attachment.size() << std::endl;
  // set resp attachment
  coro_rpc::get_context()->set_response_attachment2(attachment);
  return arg;
}
int main() {
  // Get CUDA devices and Init CUDA environment(may has latency)
  auto cuda_dev_list = coro_io::cuda_device_t::get_cuda_devices();
  assert(cuda_dev_list->size() > 0);
  for (auto &e : *cuda_dev_list) {
    std::cout << "GPU " << e->name() << ":" << e->get_gpu_id() << std::endl;
  }
  // Create an IB device which use GPU Memory as buffer
  auto dev = coro_io::ib_device_t::create(
      {.buffer_pool_config = {.gpu_id = 0 /* GPU ID */}});

  // Create and configure RPC server with IB support
  coro_rpc::coro_rpc_server server(1, 9001);
  server.init_ibv({.device = dev});  // Initialize GDR

  // Register handler function
  server.register_handler<echo>();
  server.async_start();  // Start server asynchronously

  coro_rpc::coro_rpc_client client;
  auto _ = client.init_ibv({.device = dev});
  async_simple::coro::syncAwait(client.connect("127.0.0.1", "9001"));

  // create a gpu attachment
  std::string gpu_attachment = "This gpu buffer will be transfered to otherside without cpu";
  coro_io::heterogeneous_buffer req_attachment(gpu_attachment.size(),
                                               /*gpu_id */ 0);
  coro_io::cuda_copy(req_attachment.data(), req_attachment.gpu_id(),
                     gpu_attachment.data(), -1, gpu_attachment.size());

  client.set_req_attachment2(req_attachment);

  // optional: set response attachment GPU buffer manually
  // otherwise, it will be allocated automatically
  // if len of recved resp attachment is larger than 1024, it will allocated a
  // new enough buffer too
  coro_io::heterogeneous_buffer resp_buffer(/*length*/ 1024, /*gpu_id*/ 0);
  client.set_resp_attachment_buf2(resp_buffer);

  auto result = async_simple::coro::syncAwait(client.call<echo>("hello world"));
  assert(result.has_value());
  assert(result.value() == "hello world");
  auto resp_attachment = client.get_resp_attachment2();
  assert(resp_attachment.data() == resp_buffer.data());
  assert(resp_attachment.gpu_id() == resp_buffer.gpu_id());
  std::string resp_attachment_str;
  resp_attachment_str.resize(resp_attachment.size());
  coro_io::cuda_copy(resp_attachment_str.data(), -1, resp_attachment.data(),
                     resp_attachment.gpu_id(), resp_attachment.size());
  assert(resp_attachment_str == gpu_attachment);
  return 0;
}