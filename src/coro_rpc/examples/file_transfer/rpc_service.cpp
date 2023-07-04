#include "rpc_service.h"

#include <filesystem>

std::string echo(std::string str) { return str; }

void upload_file(coro_rpc::context<std::errc> conn, file_part part) {
  std::shared_ptr<std::ofstream> stream = nullptr;
  if (!conn.get_tag().has_value()) {
    stream = std::make_shared<std::ofstream>();
    auto filename = std::to_string(std::time(0)) +
                    std::filesystem::path(part.filename).extension().string();
    stream->open(filename, std::ios::binary | std::ios::app);
    conn.set_tag(stream);
  }
  else {
    stream = std::any_cast<std::shared_ptr<std::ofstream>>(conn.get_tag());
  }

  std::cout << "file part content size=" << part.content.size() << "\n";

  stream->write(part.content.data(), part.content.size());
  if (part.eof) {
    stream->close();
    conn.set_tag(std::any{});
    std::cout << "file upload finished\n";
  }

  conn.response_msg(std::errc{});
}

void download_file(coro_rpc::context<response_part> conn,
                   std::string filename) {
  std::shared_ptr<std::ifstream> stream = nullptr;
  if (!conn.get_tag().has_value()) {
    std::string actual_filename =
        std::filesystem::path(filename).filename().string();
    if (!std::filesystem::is_regular_file(actual_filename) ||
        !std::filesystem::exists(actual_filename)) {
      conn.response_msg(response_part{std::errc::invalid_argument});
      return;
    }

    stream = std::make_shared<std::ifstream>(actual_filename, std::ios::binary);
    conn.set_tag(stream);
  }
  else {
    stream = std::any_cast<std::shared_ptr<std::ifstream>>(conn.get_tag());
  }

  char buf[1024];

  size_t real_size = stream->read(buf, 1024).gcount();
  conn.response_msg(
      response_part{{}, std::string(buf, real_size), stream->eof()});

  if (stream->eof()) {
    stream->close();
    conn.set_tag(std::any{});
  }
}

std::string g_real_file = "";
std::ofstream g_file;

std::errc upload(file_part part) {
  if (g_real_file.empty()) {
    g_real_file = std::to_string(std::time(0)) +
                  std::filesystem::path(part.filename).extension().string();
    g_file.open(g_real_file, std::ios::binary | std::ios::app);
  }

  g_file.write(part.content.data(), part.content.size());
  if (part.eof) {
    g_real_file.clear();
    g_file.close();
  }

  return {};
}
