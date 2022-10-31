#pragma once
#include <coro_rpc/coro_rpc_server.hpp>
#include <fstream>
#include <string>
using namespace coro_rpc;

std::string echo(std::string str);

struct dummy {
  std::string echo(std::string str) { return str; }
};

struct file_part {
  std::string filename;
  std::string content;
  bool eof;
};
// this function is just for case study, only allow one client to upload file.
std::errc upload(file_part part);

// support arbitrary clients to upload file.
void upload_file(coro_rpc::connection<std::errc> conn, file_part part);
