#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "async_simple/Common.h"
#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/cuda/cuda_device.hpp"
#include "ylt/coro_io/cuda/cuda_memory.hpp"
#include "ylt/coro_io/data_view.hpp"
#include "ylt/coro_io/heterogeneous_buffer.hpp"
#include "ylt/coro_io/ibverbs/ib_socket.hpp"
#include "ylt/coro_rpc/coro_rpc_client.hpp"
#include "ylt/coro_rpc/coro_rpc_server.hpp"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"
#include "ylt/easylog.hpp"

// Global counter for all clients
static std::atomic<uint64_t> total_transfers{0};
static std::chrono::steady_clock::time_point start_time;
static std::atomic<int> cnter;
std::string_view echo(std::string_view arg) {
  ++cnter;
  // get req attachement
  coro_io::data_view attachment =
      coro_rpc::get_context()->get_request_attachment2();
  std::cout << "Server received attachment gpu id:" << attachment.gpu_id()
            << '\n'
            << "size:" << attachment.size() << "cnt:" << cnter << std::endl;
  // set resp attachment
  coro_rpc::get_context()->set_response_attachment2(attachment);
  return arg;
}

void print_usage(const char* program_name) {
  std::cout
      << "Usage: " << program_name << " [options]\n"
      << "Options:\n"
      << "  --mode <server|client>    Run as server or client (default: both)\n"
      << "  --port <port>             Port number (default: 9001)\n"
      << "  --clients <num>           Number of parallel clients (default: 1)\n"
      << "  --iterations <num>        Iterations per client (default: 100)\n"
      << "  --buffer-size <MB>        Buffer size in MB (default: 128)\n"
      << "  --host <ip>               Server host IP (default: 127.0.0.1)\n"
      << "Example:\n"
      << "  " << program_name << " --mode server --port 9001\n"
      << "  " << program_name
      << " --mode client --port 9001 --clients 4 --iterations 1000\n";
}

static std::atomic<bool> server_should_stop{false};

void run_server(int port, int gpu_id) {
  std::cout << "Starting server on port " << port << " with GPU " << gpu_id
            << std::endl;

  // Create an IB device which use GPU Memory as buffer
  auto dev =
      coro_io::ib_device_t::create({.buffer_pool_config = {.gpu_id = gpu_id}});

  // Create and configure RPC server with IB support
  coro_rpc::coro_rpc_server server(1, port);
  server.init_ibv({.device = dev});

  // Register handler function
  server.register_handler<echo>();
  server.async_start();

  std::cout << "Server started, waiting for connections..." << std::endl;

  while (!server_should_stop.load(std::memory_order_relaxed)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  server.stop();
  std::cout << "Server stopped" << std::endl;
}

void run_client(const std::string& host, int port, int gpu_id, int iterations,
                size_t buffer_size, int client_id) {
  std::cout << "Client " << client_id << " connecting to " << host << ":"
            << port << std::endl;

  // Create an IB device which use GPU Memory as buffer
  auto dev =
      coro_io::ib_device_t::create({.buffer_pool_config = {.gpu_id = gpu_id}});

  coro_rpc::coro_rpc_client client;
  auto _ = client.init_ibv({.device = dev});
  async_simple::coro::syncAwait(client.connect(host, std::to_string(port)));

  std::cout << "Client " << client_id
            << " connected successfully, size:" << buffer_size << std::endl;

  // create a gpu attachment
  std::vector<coro_io::heterogeneous_buffer> req_attachment;

  // Random generator setup
  std::mt19937 gen(std::random_device{}() + client_id);
  std::uniform_int_distribution<> dis(32, 126);

  coro_io::heterogeneous_buffer resp_buffer(buffer_size, gpu_id);
  std::vector<std::string> gpu_attachment;
  for (int i = 0; i < 3; ++i) {
    req_attachment.push_back({buffer_size, gpu_id});
    gpu_attachment.emplace_back(buffer_size, '\0');
    for (size_t j = 0; j < buffer_size; ++j) {
      gpu_attachment.back()[j] = static_cast<char>(dis(gen));
    }
    coro_io::cuda_copy(
        req_attachment.back().data(), req_attachment.back().gpu_id(),
        gpu_attachment.back().data(), -1, gpu_attachment.back().size());
  }
  // Response buffer
  std::string cpu_buffer(buffer_size, '\0');
  for (int i = 0; i < iterations; ++i) {
    client.set_resp_attachment_buf2(resp_buffer);

    auto selected = std::uniform_int_distribution<>(0, 2)(gen);
    client.set_req_attachment2(req_attachment[selected]);
    auto result =
        async_simple::coro::syncAwait(client.call<echo>("hello world"));
    async_simple::logicAssert(result.has_value(), "echo call failed");
    async_simple::logicAssert(result.value() == "hello world",
                              "echo response mismatch");

    auto resp_attachment = client.get_resp_attachment2();
    std::cout << "size:" << resp_attachment.size() << std::endl;
    async_simple::logicAssert(resp_attachment.data() == resp_buffer.data(),
                              "response buffer address mismatch");
    async_simple::logicAssert(resp_attachment.gpu_id() == resp_buffer.gpu_id(),
                              "response buffer gpu_id mismatch");
    coro_io::cuda_copy(cpu_buffer.data(), -1, resp_attachment.data(),
                       resp_attachment.gpu_id(), resp_attachment.size());
    async_simple::logicAssert(
        memcmp(cpu_buffer.data(), gpu_attachment[selected].data(),
               cpu_buffer.size()) == 0,
        "cmp failed");

    uint64_t current_count = total_transfers.fetch_add(1) + 1;

    // Calculate QPS
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time)
            .count();
    double qps = elapsed_ms > 0 ? (current_count * 1000.0 / elapsed_ms) : 0.0;

    std::cout << "Total transfers: " << current_count
              << ", QPS: " << static_cast<uint64_t>(qps) << std::endl;
  }

  std::cout << "Client " << client_id << " completed all iterations"
            << std::endl;
}

int main(int argc, char* argv[]) {
  easylog::logger<>::instance().set_async(false);

  // Default values
  std::string mode = "both";
  int port = 9001;
  int num_clients = 1;
  int iterations = 100;
  size_t buffer_size_mb = 128;
  std::string host = "127.0.0.1";

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--mode" && i + 1 < argc) {
      mode = argv[++i];
    }
    else if (arg == "--port" && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    }
    else if (arg == "--clients" && i + 1 < argc) {
      num_clients = std::atoi(argv[++i]);
    }
    else if (arg == "--iterations" && i + 1 < argc) {
      iterations = std::atoi(argv[++i]);
    }
    else if (arg == "--buffer-size" && i + 1 < argc) {
      buffer_size_mb = std::atoi(argv[++i]);
    }
    else if (arg == "--host" && i + 1 < argc) {
      host = argv[++i];
    }
    else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    }
  }

  size_t buffer_size = buffer_size_mb * 1024 * 1024;

  // Get CUDA devices
  auto cuda_dev_list = coro_io::cuda_device_t::get_cuda_devices();
  if (cuda_dev_list->size() == 0) {
    std::cerr << "No CUDA devices found!" << std::endl;
    return 1;
  }

  int gpu_id = 0;
  for (auto& e : *cuda_dev_list) {
    std::cout << "GPU " << e->name() << ":" << e->get_gpu_id() << std::endl;
  }

  std::cout << "Configuration:\n"
            << "  Mode: " << mode << "\n"
            << "  Port: " << port << "\n"
            << "  Clients: " << num_clients << "\n"
            << "  Iterations: " << iterations << "\n"
            << "  Buffer size: " << buffer_size_mb << " MB\n"
            << "  Host: " << host << "\n"
            << "  GPU ID: " << gpu_id << std::endl;

  if (mode == "server") {
    run_server(port, gpu_id);
  }
  else if (mode == "client") {
    start_time = std::chrono::steady_clock::now();
    // Start multiple clients in parallel threads
    std::vector<std::thread> client_threads;
    for (int i = 0; i < num_clients; ++i) {
      client_threads.emplace_back(run_client, host, port, gpu_id, iterations,
                                  buffer_size, i);
    }

    // Wait for all clients to complete
    for (auto& t : client_threads) {
      t.join();
    }

    std::cout << "All clients completed" << std::endl;
  }
  else if (mode == "both") {
    // Run server in background thread
    std::thread server_thread(run_server, port, gpu_id);

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Initialize start time before starting clients
    start_time = std::chrono::steady_clock::now();

    // Run client(s)
    std::vector<std::thread> client_threads;
    for (int i = 0; i < num_clients; ++i) {
      client_threads.emplace_back(run_client, host, port, gpu_id, iterations,
                                  buffer_size, i);
    }

    // Wait for clients
    for (auto& t : client_threads) {
      t.join();
    }

    std::cout << "All clients completed, stopping server..." << std::endl;

    server_should_stop.store(true, std::memory_order_relaxed);
    server_thread.join();
  }
  else {
    std::cerr << "Invalid mode: " << mode << std::endl;
    print_usage(argv[0]);
    return 1;
  }

  return 0;
}
